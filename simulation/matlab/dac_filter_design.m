%% dac_filter_design.m - DAC Reconstruction Filter Design
%  Smooth stair-step DAC output back to clean sine via digital LPF.
clear; close all;
addpath(fileparts(mfilename('fullpath')));

%% ===== Parameters ======================================================

SAMPLE_RATE = 500e3;  % DAC sample rate (Hz)
AMPL_MV     = 3300;   % mVpp
OFFSET_MV   = 1650;   % mV offset
VREF_MV     = 3300;
CYCLES      = 3;

ampl_dac = voltage_to_dac(AMPL_MV, VREF_MV) / 2;
offs_dac = voltage_to_dac(OFFSET_MV, VREF_MV);

%% ===== Generate DAC Outputs ============================================

% -- 100 kHz signal (5 pts/cycle) --
f_100k  = 100e3;
n_100k  = round(SAMPLE_RATE / f_100k * CYCLES);
dac_100k_raw = generate_sine(max(n_100k, 16), ampl_dac, offs_dac);
v_100k_raw   = double(dac_100k_raw) / 4095 * VREF_MV / 1000;
t_100k       = (0:length(dac_100k_raw)-1) / SAMPLE_RATE * 1e6;  % us

% -- 1 kHz signal (500 pts/cycle) --
f_1k    = 1000;
n_1k    = round(SAMPLE_RATE / f_1k * CYCLES);
dac_1k_raw = generate_sine(max(n_1k, 16), ampl_dac, offs_dac);
v_1k_raw   = double(dac_1k_raw) / 4095 * VREF_MV / 1000;
t_1k       = (0:length(dac_1k_raw)-1) / SAMPLE_RATE * 1e3;  % ms

%% ===== Filter Design ===================================================
%  Digital Butterworth LPF, bilinear transform, zero-phase (filtfilt).

% 100 kHz signal: fc = 120 kHz (above 100k signal, below 250k Nyquist)
fc_100k = 120e3;
[b100, a100] = butter(4, fc_100k / (SAMPLE_RATE / 2));
v_100k_filt = filtfilt(b100, a100, double(v_100k_raw));

% 1 kHz signal: fc = 10 kHz (well above 1k signal)
fc_1k = 10e3;
[b1, a1] = butter(4, fc_1k / (SAMPLE_RATE / 2));
v_1k_filt_a = filtfilt(b1, a1, double(v_1k_raw));

% 1 kHz signal: tighter fc = 2 kHz (closer to signal for better noise rejection)
[b1t, a1t] = butter(4, 2000 / (SAMPLE_RATE / 2));
v_1k_filt_b = filtfilt(b1t, a1t, double(v_1k_raw));

% RC analog equivalent (1st-order): fc = 100 kHz
tau = 1 / (2 * pi * 100e3);
rc_100k = zeros(size(v_100k_raw));
rc_100k(1) = v_100k_raw(1);
for i = 2:length(v_100k_raw)
    dt = 1 / SAMPLE_RATE;
    rc_100k(i) = rc_100k(i-1) + dt/tau * (v_100k_raw(i) - rc_100k(i-1));
end

%% ===== Plot ============================================================

figure('Position', [50, 50, 1400, 900]);

% --- Row 1: 100 kHz ---
subplot(3,2,1);
stairs(t_100k, v_100k_raw, 'b-', 'LineWidth', 1.0);
grid on; xlabel('Time (us)'); ylabel('V');
title(sprintf('100 kHz DAC Raw (%.0f pts/cycle)', SAMPLE_RATE/f_100k));
ylim([-0.1, 3.4]);

subplot(3,2,2);
hold on;
stairs(t_100k, v_100k_raw, 'Color', [0.7 0.7 0.7], 'LineWidth', 0.8);
h1 = plot(t_100k, v_100k_filt, 'b-', 'LineWidth', 1.5);
h2 = plot(t_100k, rc_100k, 'r--', 'LineWidth', 1.2);
hold off; grid on; xlabel('Time (us)'); ylabel('V');
title(sprintf('Filtered: 4th-Butter(120k) + RC(100k)'));
legend([h1 h2], {'Butterworth 4th fc=120k', 'RC 1st fc=100k'}, 'Location', 'best');
ylim([-0.1, 3.4]);

% --- Row 2: 1 kHz (wide filter, fc=10k) ---
subplot(3,2,3);
stairs(t_1k, v_1k_raw, 'r-', 'LineWidth', 0.6);
grid on; xlabel('Time (ms)'); ylabel('V');
title(sprintf('1 kHz DAC Raw (%.0f pts/cycle)', SAMPLE_RATE/f_1k));
ylim([-0.1, 3.4]);

subplot(3,2,4);
hold on;
stairs(t_1k, v_1k_raw, 'Color', [0.7 0.7 0.7], 'LineWidth', 0.4);
plot(t_1k, v_1k_filt_a, 'r-', 'LineWidth', 1.5);
hold off; grid on; xlabel('Time (ms)'); ylabel('V');
title(sprintf('Filtered: 4th-Butter fc=%.0fkHz (wide)', fc_1k/1e3));
ylim([-0.1, 3.4]);

% --- Row 3: 1 kHz (tight filter, fc=2k) ---
subplot(3,2,5);
hold on;
stairs(t_1k, v_1k_raw, 'Color', [0.7 0.7 0.7], 'LineWidth', 0.4);
plot(t_1k, v_1k_filt_b, 'r-', 'LineWidth', 1.5);
hold off; grid on; xlabel('Time (ms)'); ylabel('V');
title(sprintf('Filtered: 4th-Butter fc=2kHz (tight)'));

% --- Filter frequency response ---
subplot(3,2,6);
[H100, w100] = freqz(b100, a100, 1024, SAMPLE_RATE);
[H1, w1] = freqz(b1t, a1t, 1024, SAMPLE_RATE);
semilogx(w100/1e3, 20*log10(abs(H100)), 'b-', 'LineWidth', 1.2); hold on;
semilogx(w1/1e3, 20*log10(abs(H1)), 'r-', 'LineWidth', 1.2);
xline(100, 'b--', '100 kHz sig'); xline(1, 'r--', '1 kHz sig');
xline(250, 'k--', 'Nyquist');
hold off; grid on;
xlabel('Frequency (kHz)'); ylabel('|H| (dB)');
title('Filter Frequency Response');
legend('100k filter (fc=120k)', '1k filter (fc=2k)', 'Location', 'southwest');
ylim([-60, 5]); xlim([0.1, 260]);

sgtitle('DAC Reconstruction Filter Design');

%% ===== Print filter coefficients (for C implementation) ===============

fprintf('\n=== Filter Coefficients (for firmware) ===\n');
fprintf('100 kHz signal - 4th-order Butterworth fc=%.1f kHz:\n', fc_100k/1e3);
fprintf('  b = ['); fprintf('%.8f ', b100); fprintf(']\n');
fprintf('  a = ['); fprintf('%.8f ', a100); fprintf(']\n\n');
fprintf('1 kHz signal - 4th-order Butterworth fc=2.0 kHz:\n');
fprintf('  b = ['); fprintf('%.8f ', b1t); fprintf(']\n');
fprintf('  a = ['); fprintf('%.8f ', a1t); fprintf(']\n');

fprintf('\nRC analog (1st-order) fc=100 kHz:\n');
fprintf('  R*C = %.3e  (e.g. R=1.59 kohm, C=1nF)\n', tau);
