# IMM Kalman Filter for the Pedal Force (HOLD / MOVE)

Implementation: [`ESP32/src/SignalFilter_IMM.cpp`](../../ESP32/src/SignalFilter_IMM.cpp), class `KalmanFilterIMM`.
Selected by `kfModelOrder_u8 = 4` ("IMM adaptive (hold/move)" in the SimHub plugin's load cell filter dropdown).

---

## 1. Purpose

The admittance controller (`MoveByAdmittanceStrategy`) turns the measured foot force into pedal motion. Any noise
left in the force signal moves the virtual pedal. For slow noise the displacement is roughly

$$\Delta x \approx \frac{\Delta F_{\text{noise}}}{k_{\text{local}}}$$

This is largest at light force, where the force curve is flattest. The foot yields to the motion, the load cell
reading changes, and the loop closes. The result is a subtle vibration while holding the pedal still.

A fixed linear filter can only trade noise against lag, and lag inside the admittance loop makes the pedal feel
heavy or unstable. The IMM (Interacting Multiple Model) filter instead runs two hypotheses about the foot force
and blends them by how well each explains the incoming samples:

| Model | Assumption | Behaviour |
|---|---|---|
| **HOLD** (index 0) | force is constant (random walk) | strong smoothing, ≈ 1.5 Hz |
| **MOVE** (index 1) | force changes at a rate (constant rate) | low lag, 5–60 Hz (slider) |

The admittance model itself stays deterministic. Its states are our own integrator and are known exactly, so the
only uncertain quantity, and the only one the filter estimates, is the human force.

---

## 2. Position in the signal chain

```text
load cell task ──queue──► loadcellReading [kg]
                            │  convertToPedalForce(·, sledPosition)   (linear: F = g(x_sled) · F_lc)
                            ▼
                     pedalForce_fl32 [kg]
                            │  KalmanFilterIMM::filteredValue(z, newSample, Δt, slider)
                            ▼
                     filteredReading [kg] ──► MoveByAdmittanceStrategy(...)
                     changeVelocity  [kg/s]
```

`newSample` is true only in cycles where the load cell queue delivered a fresh value. In the other cycles the
filter only predicts; held values are never used as new measurements.

---

## 3. State and models

Both models share the state vector, so their estimates can be mixed:

$$\mathbf{x} = \begin{bmatrix} F \\ \dot F \end{bmatrix} \qquad [\text{kg},\ \text{kg/s}]$$

**Measurement model** (both models):

$$z_k = \mathbf{H}\,\mathbf{x}_k + v_k,\qquad \mathbf{H} = \begin{bmatrix} 1 & 0 \end{bmatrix},\qquad v_k \sim \mathcal{N}(0, R)$$

### 3.1 HOLD model: random walk on the force

$$\mathbf{F}_H = \begin{bmatrix} 1 & 0 \\ 0 & 0 \end{bmatrix},\qquad
\mathbf{Q}_H = \begin{bmatrix} q_H\,\Delta t & 0 \\ 0 & \varepsilon \end{bmatrix}$$

The force rate is forced to zero. $\varepsilon = 10^{-9}$ only keeps $\mathbf{P}$ positive definite.

### 3.2 MOVE model: constant force rate (white noise on $\dot F$)

$$\mathbf{F}_M = \begin{bmatrix} 1 & \Delta t \\ 0 & 1 \end{bmatrix},\qquad
\mathbf{Q}_M = q_M \begin{bmatrix} \tfrac{\Delta t^3}{3} & \tfrac{\Delta t^2}{2} \\[2pt] \tfrac{\Delta t^2}{2} & \Delta t \end{bmatrix}$$

$\mathbf{Q}_M$ is the exact discretization of continuous white noise with spectral density $q_M$ acting on $\dot F$.

---

## 4. Noise parameters

### 4.1 Measurement noise $R$

At startup, `loadcell->estimateBiasAndVariance()` measures the load cell variance $\sigma^2_{lc}$ in kg². The
conversion to pedal force is linear in the load cell force, with a gain that depends on the sled position:

$$g(x_{\text{sled}}) = \texttt{convertToPedalForce}(1,\ x_{\text{sled}})$$

$R$ is updated every cycle to that gain:

$$R = \sigma^2_{lc}\; g(x_{\text{sled}})^2$$

### 4.2 Process noise from target bandwidths

For a Kalman filter at steady state, the measurement noise with variance $R$ sampled every $T$ seconds is
equivalent to a continuous noise density

$$r = R\,T$$

where $T$ is the averaged load cell sample interval (section 6.4). For the continuous-time (Kalman–Bucy) steady
state of each model:

- **Random walk (HOLD):** first-order low pass with $\omega_H = \sqrt{q_H / r}$, so

$$q_H = R\,T\,\omega_H^2,\qquad \omega_H = 2\pi f_H,\quad f_H = 1.5\ \text{Hz}$$

- **Constant rate (MOVE):** second-order low pass with $\omega_n = (q_M / r)^{1/4}$ and damping $\zeta = 1/\sqrt 2$, so

$$q_M = R\,T\,\omega_M^4,\qquad \omega_M = 2\pi f_M$$

Specifying the noise as bandwidths keeps the filter's behaviour the same regardless of load cell noise level and
sample rate.

### 4.3 Slider mapping

The existing "Loadcell denoising" slider $s \in [1, 255]$ (`kfModelNoise_u8`) selects the MOVE bandwidth on a
logarithmic scale:

$$f_M = f_{\min}\left(\frac{f_{\max}}{f_{\min}}\right)^{\frac{s-1}{254}},\qquad f_{\min} = 5\ \text{Hz},\ f_{\max} = 60\ \text{Hz}$$

---

## 5. Mode switching (Markov chain)

The mode is modelled as a Markov chain with an expected dwell time $\tau_j$ in each mode:

$$p_{jj} = e^{-\Delta t / \tau_j},\qquad p_{ij} = 1 - p_{ii}\ \ (i \neq j)$$

$$\boldsymbol{\Pi} = \begin{bmatrix} p_{HH} & 1 - p_{HH} \\ 1 - p_{MM} & p_{MM} \end{bmatrix},\qquad \tau_H = 0.3\ \text{s},\ \ \tau_M = 0.15\ \text{s}$$

$p_{ij}$ is the probability of being in model $j$ now, given model $i$ in the previous cycle.

---

## 6. One IMM cycle

Inputs: the previous estimates $\hat{\mathbf{x}}^i,\ \mathbf{P}^i$ and mode probabilities $\mu^i$ for
$i \in \{H, M\}$, plus the measurement $z$.

### 6.1 Interaction (mixing)

Predicted mode probabilities:

$$\bar c_j = \sum_i p_{ij}\,\mu^i$$

Mixing weights (probability that the system was in model $i$, given that it is in model $j$ now):

$$\mu^{i|j} = \frac{p_{ij}\,\mu^i}{\bar c_j}$$

Mixed initial state and covariance for each model $j$:

$$\hat{\mathbf{x}}^{0j} = \sum_i \mu^{i|j}\,\hat{\mathbf{x}}^i$$

$$\mathbf{P}^{0j} = \sum_i \mu^{i|j}\left[\mathbf{P}^i + \left(\hat{\mathbf{x}}^i - \hat{\mathbf{x}}^{0j}\right)\left(\hat{\mathbf{x}}^i - \hat{\mathbf{x}}^{0j}\right)^{\!\top}\right]$$

The spread term $(\hat{\mathbf{x}}^i - \hat{\mathbf{x}}^{0j})(\cdot)^\top$ adds uncertainty when the two models disagree.

### 6.2 Prediction (per model)

$$\hat{\mathbf{x}}^j_{-} = \mathbf{F}_j\,\hat{\mathbf{x}}^{0j},\qquad
\mathbf{P}^j_{-} = \mathbf{F}_j\,\mathbf{P}^{0j}\,\mathbf{F}_j^\top + \mathbf{Q}_j$$

Written out for MOVE (as in the code):

$$\begin{aligned}
P_{00} &= P^0_{00} + \Delta t\,(P^0_{01} + P^0_{10}) + \Delta t^2 P^0_{11} + q_M \tfrac{\Delta t^3}{3} \\
P_{01} &= P_{10} = P^0_{01} + \Delta t\,P^0_{11} + q_M \tfrac{\Delta t^2}{2} \\
P_{11} &= P^0_{11} + q_M\,\Delta t
\end{aligned}$$

### 6.3 Update (per model, only if a fresh sample arrived)

Innovation and its covariance:

$$y_j = z - \hat F^j_{-},\qquad S_j = P^j_{-,00} + R$$

Kalman gain:

$$\mathbf{K}_j = \frac{1}{S_j}\begin{bmatrix} P^j_{-,00} \\ P^j_{-,10} \end{bmatrix}$$

State and covariance:

$$\hat{\mathbf{x}}^j = \hat{\mathbf{x}}^j_{-} + \mathbf{K}_j\,y_j,\qquad
\mathbf{P}^j = (\mathbf{I} - \mathbf{K}_j\mathbf{H})\,\mathbf{P}^j_{-}$$

$$\begin{aligned}
P_{00} &= (1 - K_0)\,P_{-,00} \\
P_{01} &= P_{10} = (1 - K_0)\,P_{-,01} \\
P_{11} &= P_{-,11} - K_1\,P_{-,01}
\end{aligned}$$

The diagonal terms are floored at $10^{-9}$ and $S_j$ at $10^{-12}$.

Gaussian likelihood of the innovation, evaluated in the log domain. The constant $-\tfrac12\ln 2\pi$ is dropped
because it cancels in the normalization:

$$\Lambda_j = \frac{1}{\sqrt{2\pi S_j}}\exp\!\left(-\frac{y_j^2}{2 S_j}\right)
\quad\Rightarrow\quad
\ell_j = -\tfrac12\left(\frac{y_j^2}{S_j} + \ln S_j\right)$$

Mode probability update. Subtracting $\ell_{\max} = \max_j \ell_j$ avoids float underflow:

$$\mu^j = \frac{e^{\ell_j - \ell_{\max}}\,\bar c_j}{\sum_i e^{\ell_i - \ell_{\max}}\,\bar c_i}$$

**Without a fresh sample**, only the Markov prior applies: $\mu^j = \bar c_j$.

### 6.4 Sample interval estimate

The time since the last fresh sample, $t_s$, is accumulated each cycle. On every fresh sample it updates the
averaged interval $T$ used in section 4.2:

$$T \leftarrow T + \alpha\,(t_s - T),\qquad \alpha = 0.05$$

### 6.5 Probability floor

Each mode probability is kept at or above $\mu_{\min} = 10^{-3}$ and then renormalized, so the inactive model can
win back within a few samples:

$$\mu^j \leftarrow \frac{\max(\mu^j, \mu_{\min})}{\sum_i \max(\mu^i, \mu_{\min})}$$

### 6.6 Output

The output is the probability-weighted combination of both models:

$$\hat F = \sum_j \mu^j\,\hat F^j \quad(\texttt{filteredReading}),\qquad
\hat{\dot F} = \sum_j \mu^j\,\hat{\dot F}{}^j \quad(\texttt{changeVelocity})$$

---

## 7. Why it separates noise from real presses

- **Holding still:** both models see zero-mean innovations. HOLD predicts more tightly ($S_H < S_M$), so every
  sample adds about $\tfrac12 \ln(S_M / S_H)$ to the log-likelihood ratio in favour of HOLD. $\mu^M$ decays toward
  $\mu_{\min}$, and the output follows the 1.5 Hz HOLD estimate.
- **Pressing:** a force ramp at rate $\dot F$ builds a lag in HOLD of about $\dot F / \omega_H$. Once $|y_H|$
  exceeds a few $\sqrt{S_H}$, the model with the larger $S$ (MOVE) explains the samples better. $\mu^M \to 1$
  within a few samples, and the output switches to the low-lag MOVE estimate.

---

## 8. Parameters

| Constant (in `SignalFilter_IMM.cpp`) | Value | Effect |
|---|---|---|
| `s_holdBandwidthHz_fl32` | 1.5 Hz | smoothing while holding; higher = less "sticky" |
| `s_moveBandwidthMinHz_fl32` / `MaxHz` | 5 / 60 Hz | MOVE bandwidth range covered by the slider |
| `s_holdDwellTime_s_fl32` | 0.3 s | expected HOLD duration; longer = fewer switches |
| `s_moveDwellTime_s_fl32` | 0.15 s | expected MOVE duration |
| `s_modeProbabilityMin_fl32` | 1e-3 | floor that keeps both models alive |
| `s_sampleIntervalFilterAlpha_fl32` | 0.05 | smoothing of the measured sample interval |

---

## 9. Telemetry and tuning

While mode 4 is active, the extended pedal state field `oscillationMonitorValue_u8` carries $255\,\mu^M$. It
appears as the "Oscillation Monitor" signal in the live plot: 0 means HOLD, 255 means MOVE.

| Symptom | Adjustment |
|---|---|
| Residual vibration while holding | lower `s_holdBandwidthHz_fl32` |
| Holding feels sticky or delayed on slow presses | raise `s_holdBandwidthHz_fl32` |
| Monitor flickers during slow, steady presses | lengthen both dwell times |
| Presses feel laggy | raise the slider (higher MOVE bandwidth) |

---

## 10. Simulation check

The filter was checked with a Python port of the same equations on synthetic data: σ = 8 g load cell noise, a
2 kHz load cell and a 4 kHz control loop. These are simulation results, not measurements on hardware.

| Slider | Holding still: output noise σ | MOVE detected after a 200 kg/s press starts | Slow ramp (4 kg/s): bias |
|---|---|---|---|
| 1 | 0.4–0.8 g | 1.0 ms | < 1 g |
| 128 | 0.3–1.2 g | 0.5 ms | < 1 g |
| 255 | 0.5–1.0 g | 0.5 ms | < 1 g |

---

## References

- H. A. P. Blom, Y. Bar-Shalom, *The Interacting Multiple Model Algorithm for Systems with Markovian Switching
  Coefficients*, IEEE Transactions on Automatic Control, 33(8), 1988.
- Y. Bar-Shalom, X. R. Li, T. Kirubarajan, *Estimation with Applications to Tracking and Navigation*, Wiley, 2001
  (chapter 11: IMM; chapter 6: discretized white-noise models).
