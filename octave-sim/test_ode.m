% Matlab Code for Series RLC Circuit modeling..
% Praviraj PG (pravirajpg@gmail.com) - Rev 0, 19-Sep-2019
% Solve using ODE45 function with the State-Space Model
clear all;
close all;
pkg load signal;

%%
% Model input voltage
%%
function y = vi(t)
  y = 50*square(2*pi*t/0.25) + 50;
end

%%
% Model Series RLC Circuit function.
%%
function y = RLCcktfcn(t,x)
    R = 20;               % Resistance (Series RL Branch), Ohms..
    L = 100e-3;           % Inductance (Series RL Branch), H..
    C = 50e-6;            % Capacitor (Series RLC Branch), F
    Vi = vi(t);           % Input Voltage
    % RLC Circuit Dynamic - State-Space Model..
    % y(1) = d (Ii);    y(2) = d (Vc);
    %        /dt               /dt
    y = [-R/L, -1/L; 1/C, 0]*x + [1/L; 0]*Vi;
end

T = 0.5;           % Simulation Time..
x0 =  [0; 0];      % Initial Conditions
% Call MATLAB/Octave ODE45 for Solving..
% A State-Space model of Series RLC Circuit is implemented inside 
% the function "RLCcktfcn" below
tspan = [0, T];
[t, x] = ode45(@(t,x) RLCcktfcn(t,x), tspan, x0,
  odeset('RelTol', 1e-4, 'AbsTol', 1e-6)); 
% Plot Current thru Inductor & Voltage across Capacitor
Ii = x(:,1);    Vc = x(:,2);

subplot(2,1,1); plot(t, Vc, 'r-', 'linewidth',0.75); grid on;
hold on;  plot(t, vi(t), 'b-', 'linewidth',0.75);
title('Series RLC Circuit - Voltages');
legend('Vc', 'Vi');
xlabel('Time (sec)'); ylabel('Voltage (V)');
    
subplot(2,1,2); plot(t, Ii, 'r-', 'linewidth',0.75); grid on;
title('Series RLC Circuit - Current'); 
xlabel('Time (sec)'); ylabel('Current (A)');
