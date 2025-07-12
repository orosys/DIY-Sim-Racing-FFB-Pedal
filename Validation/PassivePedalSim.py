# import libraries
import numpy as np
from scipy.integrate import solve_ivp
import matplotlib.pyplot as plt

########################################################################################################################
# system parameters
m_p = 1e3       # pedal mass in kg
k_p = 1e6     # foot stiffness in N/m
zeta = 0.5 # 0 = no damping Dämpfung; 1 = critical damping, >1 = overdamping
b_critical = 2 * np.sqrt(m_p * k_p)
b_p = b_critical * zeta # damping coefficient in Ns/m

# polynomial coefficients to parameterize pedals force-travel : F_target(x) = a*x + b*x^2 + c*x^3
a, b, c = 10000.0, 300000.0, 0.0

# limits
rpm_max = 2*250000 / 6400 * 60 # in rev per minute
spindle_ptch = 0.01 # in meter

x_min, x_max = -0.0, 0.1         # 0–100 mm pedal travel. Value given in m
v_max = rpm_max * spindle_ptch / 60 # in m/s
a_max = 500.0                     # [m/s²]

########################################################################################################################
# User input as position
def user_position(t):
    return 0.05 if 0.2 <= t <= 1.5 else 0.0

# User input as force
def user_force(t):
    return 500.0 if 0.2 <= t <= 1.5 else 0.0

# Compute force-travel curve
def F_target(x):
    # example values
    # travel = 10mm = 0.01m
    # force at max travel = 100kg = 100kg * 9.81m/s^2 = 9810N
    #a = 9810 / 0.1 = 9810 ca 1e4
    force =  a * x + b * x**2 + c * x**3
    gradient =  a  + 2 * b * x + 3* c * x**2
    return [force, gradient]

# soft endstop
def saturation_force(x):
    if x < x_min:
        return -1e7 * (x - x_min) # when x < x_min --> (x - x_min) < 0 --> positive acceleration needed
    elif x > x_max:
        return -1e7 * (x - x_max) # when x > x_max --> (x - x_max) > 0 --> negative acceleration needed
    else:
        return 0.0

# ODE-System
def pedal_dynamics(t, y):
    # ODE solver returns the integrated return values -->
    # [x;v] = [position, velocity]
    x, v = y  

    # soft endstops additional force
    F_sat = saturation_force(x)

    x_user = user_position(t)
    F_desired, F_gradient = F_target(x)

    # Dynamic equation (Newton 2)
    # solved for highest derivative
    a_pedal = (-F_desired - b_p * v - k_p * (x - x_user) + F_sat) / m_p

    # limiting the acceleration
    a_pedal = np.clip(a_pedal, -a_max, a_max)

    # limiting the velocity
    v = np.clip(v, -v_max, v_max)

    # return the first order system values
    # [d(x)/d(t); d(v)/d(t)] = [v; a]
    return [v, a_pedal]

########################################################################################################################
# Starting states: [Position, velocity]
y0 = [0.0, 0.0]

# Simulation time adjustment
t_span = (0, 2.0)
t_eval = np.linspace(*t_span, 2000)

# solve ODE
abserr = 1.e-8
relerr = 1.e-8
solution = solve_ivp(pedal_dynamics, t_span, y0, t_eval=t_eval,
                     rtol=relerr, atol=abserr)

# Extract results
t_res = solution.t
x_res = solution.y[0]
v_res = solution.y[1]

# obtain derivatives
a_res = np.zeros(np.size(t_res))
x_target = np.zeros(np.size(t_res))
f_desired_res = np.zeros(np.size(t_res))
f_spring_res = np.zeros(np.size(t_res))
f_damping_res = np.zeros(np.size(t_res))
f_mass_res = np.zeros(np.size(t_res))
for idx in range(np.size(t_res)):
    tmp = pedal_dynamics(solution.t[idx], solution.y[:,idx])
    a_res[idx] = tmp[1]
    x_target[idx] = user_position(solution.t[idx])
    f_desired_res[idx], _ = F_target(x_res[idx])
    f_spring_res[idx] = k_p * (x_res[idx] - x_target[idx])
    f_damping_res[idx] = b_p * v_res[idx]
    f_mass_res[idx] = m_p * a_res[idx]

########################################################################################################################
# Plot
plt.figure(figsize=(12, 8))

# position based plots
plt.subplot(3, 2, 1)
plt.plot(t_res, x_res * 1000, label='pedal travel')
plt.plot(t_res, x_target * 1000, label='user induced travel')
plt.plot(t_res, (x_target - x_res) * 1000, label='foot compression')
plt.ylabel('pedal travel in mm')
plt.grid()
plt.xlabel('time in sec')
plt.legend()

plt.subplot(3, 2, 3)
plt.plot(t_res, v_res * 1000)
plt.ylabel('pedal velocity in mm/s')
plt.grid()
plt.xlabel('time in sec')

plt.subplot(3, 2, 5)
plt.plot(t_res, a_res * 1000)
plt.ylabel('pedal acceleratrion in mm/s^2')
plt.grid()
plt.xlabel('time in sec')

# force plots
plt.subplot(3, 2, 2)
x_eval = np.linspace(0, x_max, 1000)
F_force_travel, F_target_gradient = F_target(x_eval)
plt.plot(x_eval * 1000, F_force_travel)
plt.ylabel('force in N')
plt.grid()
plt.xlabel('travel in mm')
plt.title('parameterized force-travel curve')

plt.subplot(3, 2, 4)
plt.plot(t_res, f_spring_res, label='F_spring')
plt.plot(t_res, f_desired_res, '--', label='F_target(x)')
plt.plot(t_res, f_damping_res, label='F_damper')
plt.plot(t_res, f_mass_res, label='F_mass')
plt.ylabel('force in N')
plt.legend()
plt.grid()
plt.xlabel('time in sec')

plt.tight_layout()
plt.show()
