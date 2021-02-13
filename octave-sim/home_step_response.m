% Matlab Code for Series RLC Circuit modeling..
% Praviraj PG (pravirajpg@gmail.com) - Rev 0, 19-Sep-2019
% Solve using ODE45 function with the State-Space Model
clear all;
close all;
pkg load signal;

%%
% Global variables
%%
global T = 30*60;          % Simulation Time.
min_samples = 100;
ode_options = odeset('RelTol', 1e-4, 'AbsTol', 1e-6, 'MaxStep', T/min_samples);

%%
% Model input signal
%%
function y = si(t)
  global T;
  T_on = 15*60;
  y = 0.5*square(2*pi*t/T, 100*T_on/T)+0.5;
end

%%
% Model Series RLC Circuit function.
%%
function y = home_ssm(t,x)
  Cw = 3;
  Rwa = 250;
  Ca = 60;
  Ra = 400;
  Ii = si(t);
  
  % x is in the form [Vw ; Va]
  % y is in the form [dVw/dt ; dVa/dt]
  A = [-1/(Cw*Rwa) , 1/(Cw*Rwa) ; 1/(Ca*Rwa) , -1*(Ra + Rwa)/(Ca*Ra*Rwa)];
  y = A*x + [1/Cw ; 0]*Ii;
  
  % If we reached max water temp stop heating
  if ((x(1) >= 55) && (y(1) > 0))
    y(1) = 0;
  endif
end

x0 =  [16; 16];     % Initial Conditions
tspan = [0, T];
% solve
[t, x] = ode45(@(t,x) home_ssm(t,x), tspan, x0, ode_options);
  
% Plot stuff
Vw = x(:,1);    Va = x(:,2);

subplot(2,1,1);
plot(t/60, si(t).*ones(1, length(t)), 'b-', 'linewidth', 0.75);
grid on;
title('Home - Input Power');
xlabel('Time (min)'); ylabel('Power (A)');

subplot(2,1,2);
plot(t/60, Vw, 'b-', 'linewidth', 0.75);
grid on;
title('Home - Water Temperature');
xlabel('Time (min)'); ylabel('Voltage (V)');

figure();
plot(t/60, Va, 'b-', 'linewidth', 0.75);
grid on;
title('Home - Ambient Temperature');
xlabel('Time (min)'); ylabel('Voltage (V)');