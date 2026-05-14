%% dac_dual_sine.m — DAC Sine: 100kHz vs 1kHz
%  Compare high-speed vs low-speed sine output quality
clear; close all;
addpath(fileparts(mfilename('fullpath')));

AMPL_MV   = 3300;
OFFSET_MV = 1650;
VREF_MV   = 3300;
SAMPLE_RATE = 500e3;  % 500 ksps DAC output rate

amplitude_dac = voltage_to_dac(AMPL_MV, VREF_MV) / 2;
offset_dac    = voltage_to_dac(OFFSET_MV, VREF_MV);

%% ---- 100 kHz sine (5 points per cycle at 500 ksps) ----
f1 = 100e3;
n1 = round(SAMPLE_RATE / f1 * 3);  % 3 cycles
dac1 = generate_sine(max(n1, 16), amplitude_dac, offset_dac);
v1   = double(dac1) / 4095 * VREF_MV / 1000;
t1   = (0:length(dac1)-1) / SAMPLE_RATE * 1e6;  % us

%% ---- 1 kHz sine (500 points per cycle at 500 ksps) ----
f2 = 1000;
n2 = round(SAMPLE_RATE / f2 * 3);
dac2 = generate_sine(max(n2, 16), amplitude_dac, offset_dac);
v2   = double(dac2) / 4095 * VREF_MV / 1000;
t2   = (0:length(dac2)-1) / SAMPLE_RATE * 1e3;  % ms

%% ---- Plot ----
figure('Position', [100, 100, 1100, 500]);

subplot(1,2,1);
stairs(t1, v1, 'b-', 'LineWidth', 1.2);
grid on; xlabel('Time (us)'); ylabel('Voltage (V)');
title(sprintf('100 kHz Sine (%.0f pts/cycle)', SAMPLE_RATE/f1));
ylim([-0.1, 3.4]);

subplot(1,2,2);
stairs(t2, v2, 'r-', 'LineWidth', 0.6);
grid on; xlabel('Time (ms)'); ylabel('Voltage (V)');
title(sprintf('1 kHz Sine (%.0f pts/cycle)', SAMPLE_RATE/f2));
ylim([-0.1, 3.4]);

sgtitle('DAC Sine Output: 100 kHz vs 1 kHz (12-bit, Vref=3.3V)');
fprintf('100 kHz: %d pts, step=%.2f us  |  1 kHz: %d pts, step=%.2f us\n', ...
        length(dac1), 1e6/SAMPLE_RATE, length(dac2), 1e3/SAMPLE_RATE);
