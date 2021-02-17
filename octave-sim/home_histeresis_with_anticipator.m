% describe me
clear all;
close all;
pkg load signal;

%%
% Global variables
%%
global T_SIM_MINS = 2*60; % Simulation Time (minutes).
global T_SAMPLE = 1*60; % Sample duration (seconds). Each sample a new input
                          % value will arrive from the controller
min_samples = 3;
ode_options = odeset('RelTol', 1e-2, 
                     'AbsTol', 1e-4,
                     'MaxStep', T_SAMPLE/min_samples);

%%
% Model input signal
%%
function y = si(t)
  y = 17.5;
end

%%
% Model the controller for the home heating system.
% Receives the input signal (target temperature) and home temperature
% Outputs the heating state as 0 (off) or 1 (on).
%%
function ht_en = run_controller(T_target, T_home, t)
  global final_temp = T_target; % keep a global state
  global anticipator_temp = 0;
  
  hysteresis = 0.5;
  anticipator_rate = 0.2;
  
  if (T_home < final_temp)
    ht_en = 1;
    anticipator_temp += anticipator_rate;
    final_temp = T_target + hysteresis - anticipator_temp;
  else
    ht_en = 0;
    anticipator_temp -= anticipator_rate;
    anticipator_temp = max(0, anticipator_temp);
    final_temp = T_target - hysteresis - anticipator_temp;
  endif
end

%%
% Model the home + heating system as a state-space representation
%
% x is in the form [Vw ; Va]
% x_dot is in the form [dVw/dt ; dVa/dt]
% heater_state can be either 0 (off) or 1 (on)
%%
function x_dot = home_ssm(t, x, heater_state)
  Cw = 3;
  Rwa = 300;
  Ca = 30;
  Ra = 350;
  MAX_WATER_TEMP = 50;
  
  I_pow = 1*heater_state;%run_controller(si(t), x(2), t); % input power (normalized)

  A = [-1/(Cw*Rwa) ,         1/(Cw*Rwa);
        1/(Ca*Rwa) , -1*(Ra + Rwa)/(Ca*Ra*Rwa)];
  x_dot = A*x + [1/Cw ; 0]*I_pow;
  
  % If we reached max water temp stop heating
  if ((x(1) >= MAX_WATER_TEMP) && (x_dot(1) > 0))
    x_dot(1) = 0;
  endif
end

x0 = [16; 16];     % Initial Conditions
t0 = 0;
tspan = [0, T_SAMPLE];
t_tot = [];
x_tot = [];
ht_tot = [];
% solve
for i = 1:(T_SIM_MINS*60/T_SAMPLE)
  ht_en = run_controller((si(t0)+rand()-0.5), x0(2), t0);
  [t, x] = ode45(@(t,x) home_ssm(t,x, ht_en), tspan, x0, ode_options);
  x0 = x(end, :);
  t0 = t(end);
  tspan = [t0, t0 + T_SAMPLE];
  t_tot = [t_tot; t];
  x_tot = [x_tot; x];
  ht_tot = [ht_tot; [t0, ht_en]];
endfor

% Plot stuff
Vw = x_tot(:,1);    Va = x_tot(:,2);

figure();
plot(t_tot/60, Vw, 'b-', 'linewidth', 0.75);
grid on;
title('Home - Water Temperature');
xlabel('Time (min)'); ylabel('Voltage (V)');

figure();
subplot(2,1,1);
plot(ht_tot(:,1)/60, ht_tot(:,2), 'b-', 'linewidth', 0.75);
xlabel('Time (min)'); ylabel('Heater state');
grid on;
title('Home - Heater State');
subplot(2,1,2);
plot(t_tot/60, si(t_tot).*ones(1, length(t_tot)), 'k--', 'linewidth', 0.75);
hold on;
plot(t_tot/60, Va, 'b-', 'linewidth', 0.75);
grid on;
title('Home - Ambient Temperature');
xlabel('Time (min)'); ylabel('Voltage (V)');
energy = sum(ht_tot(:,2))
temp_avg = sum(Va) / length(Va)