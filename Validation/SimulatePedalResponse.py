import numpy as np
from scipy.integrate import solve_ivp
import matplotlib.pyplot as plt

# used ChatGPT for help, see https://chatgpt.com/share/6822e03f-87b0-8012-afe8-0a1ccc3806e4

# Systemparameter
m_p = 1       # Pedalmasse [kg]
b_p = 4000.0       # Dämpfung [Ns/m]
k_p = 5000.0     # Mechanische Rückstellfeder [N/m]

# Ziel-Kraft-Weg-Kennlinie: F_target(x) = a*x + b*x^2 + c*x^3
a, b, c = 10000.0, 300000.0, 0.0

# PI-Reglerparameter
K_p = 20.     # Proportionalanteil
K_i = .2     # Integralanteil

# Begrenzungen

rpm_max = 250000 / 6400 * 60 # in rev per minute
spindle_ptch = 0.01 # in meter

x_min, x_max = -0.0, 0.05         # [m] = 0–50 mm Pedalweg
v_max = rpm_max * spindle_ptch / 60 #0.5                      # [m/s] = 500 mm/s
a_max = 5.0                     # [m/s²]


# Eingabekraft durch Benutzer (Wägezelle-Simulation)
def user_force(t):
    return 500.0 if 0.2 <= t <= 1.5 else 0.0  # Benutzer drückt kurz das Pedal

# Ziel-Kraft-Weg-Kennlinie
def F_target(x):
    # example values
    # travel = 10mm = 0.01m
    # force at max travel = 100kg = 100kg * 9.81m/s^2 = 9810N
    return a * x + b * x**2 + c * x**3

# ODE-System
def pedal_dynamics(t, y):
    x, v, integral_error = y  # Position, Geschwindigkeit, Integralregler

    # Optional: Anschlagprüfung
    if x < x_min and v < 0:
        v = 0
        a_pedal = 0
        return [0.0, 0.0, 0.0]
    elif x > x_max and v > 0:
        v = 0
        a_pedal = 0
        return [0.0, 0.0, 0.0]

    F_user = user_force(t)
    F_desired = F_target(x)
    error = -(F_desired - F_user)

    # PI-Reglerausgang
    F_servo = K_p * error + K_i * integral_error
    F_servo = np.clip(F_servo, -1000, 1000)  # Begrenze auf realistische Werte

    # Dynamikgleichung (Newton 2)
    a_pedal = (F_user + F_servo - b_p * v - k_p * x) / m_p

    # Begrenzung der Beschleunigung
    a_pedal = np.clip(a_pedal, -a_max, a_max)

    # Begrenzung der Geschwindigkeit (Rückgabewert)
    v = np.clip(v, -v_max, v_max)

    return [v, a_pedal, error]

# Anfangsbedingungen: [Position, Geschwindigkeit, Integralzustand]
y0 = [0.0, 0.0, 0.0]

# Simulationszeitraum
t_span = (0, 2.0)
t_eval = np.linspace(*t_span, 2000)

# Numerische Lösung
solution = solve_ivp(pedal_dynamics, t_span, y0, t_eval=t_eval)

# Ergebnisse extrahieren
x = solution.y[0]
v = solution.y[1]
e_int = solution.y[2]
t = solution.t
F_user_vals = np.array([user_force(ti) for ti in t])
F_target_vals = F_target(x)
F_error = F_target_vals - F_user_vals
F_servo_vals = K_p * F_error + K_i * e_int


# Beschleunigung nachträglich berechnen
velocity = solution.y[1]
acceleration = (F_user_vals + F_servo_vals - b_p * velocity - k_p * x) / m_p

# Plot
plt.figure(figsize=(12, 8))

# position based plots
plt.subplot(3, 2, 1)
plt.plot(t, x * 1000)
plt.ylabel('Pedalweg [mm]')
plt.grid()
plt.xlabel('Zeit [s]')

plt.subplot(3, 2, 3)
plt.plot(t, v * 1000)
plt.ylabel('Pedalgeschwindigkeit [mm/s]')
plt.grid()
plt.xlabel('Zeit [s]')

plt.subplot(3, 2, 5)
plt.plot(t, acceleration * 1000)
plt.ylabel('Pedalwegbeschleunigung [mm/s^2]')
plt.grid()
plt.xlabel('Zeit [s]')




# force plots
plt.subplot(3, 2, 2)
x_eval = np.linspace(0, x_max, 1000)
F_force_travel = F_target(x_eval)
plt.plot(x_eval * 1000, F_force_travel, label='F_curve')
plt.ylabel('Kraft [N]')
plt.legend()
plt.grid()
plt.xlabel('Weg [mm]')

plt.subplot(3, 2, 4)
plt.plot(t, F_user_vals, label='F_user')
plt.plot(t, F_target_vals, '--', label='F_target(x)')
plt.plot(t, F_servo_vals, label='F_servo')
plt.ylabel('Kraft [N]')
plt.legend()
plt.grid()
plt.xlabel('Zeit [s]')

plt.subplot(3, 2, 6)
plt.plot(t, F_error)
plt.ylabel('Regelabweichung [N]')
plt.xlabel('Zeit [s]')
plt.grid()

plt.tight_layout()
plt.show()
