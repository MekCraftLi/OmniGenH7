%% dac_sim.m - OmniGenH7 DAC Output Simulation
%  All algorithms match firmware (src/domain/waveform_synthesis.cpp) exactly.
clear; close all;
addpath(fileparts(mfilename('fullpath')));

%% -------- Config -------------------------------------------------------

WAVEFORM = 'sine';     % 'sine' | 'square' | 'triangle' | 'sawtooth'
FREQ_HZ  = 1000;       % Signal frequency (Hz)
SAMPLE_RATE = 100e3;   % DAC sample rate (Hz)
CYCLES    = 3;         % Number of cycles to display
AMPL_MV   = 3300;      % Peak-to-peak amplitude (mV)
OFFSET_MV = 1650;      % DC offset (mV), center = Vref/2
DUTY      = 500;       % Duty cycle permille (square only), 500 = 50%
VREF_MV   = 3300;      % DAC reference voltage (mV)

%% -------- Generate -----------------------------------------------------

n_total  = round(SAMPLE_RATE / FREQ_HZ * CYCLES);
n_total  = max(n_total, 16);

amplitude_dac = voltage_to_dac(AMPL_MV, VREF_MV) / 2;
offset_dac    = voltage_to_dac(OFFSET_MV, VREF_MV);

switch lower(WAVEFORM)
    case 'sine'
        dac = generate_sine(n_total, amplitude_dac, offset_dac);
    case 'square'
        dac = generate_square(n_total, amplitude_dac, offset_dac, DUTY);
    case 'triangle'
        dac = generate_triangle(n_total, amplitude_dac, offset_dac);
    case 'sawtooth'
        dac = generate_sawtooth(n_total, amplitude_dac, offset_dac);
    otherwise
        error('Unknown waveform: %s', WAVEFORM);
end

v_out = double(dac) / 4095 * VREF_MV / 1000;  % DAC code -> voltage (V)
t_ms  = (0:n_total-1) / SAMPLE_RATE * 1000;   % Time axis (ms)

%% -------- Plot ---------------------------------------------------------

stairs(t_ms, v_out, 'LineWidth', 1.2);
grid on;
xlabel('Time (ms)');
ylabel('Voltage (V)');
title(sprintf('DAC Output - %s %d Hz (Ampl=%.1fVpp Offset=%.1fV)', ...
      WAVEFORM, FREQ_HZ, AMPL_MV/1000, OFFSET_MV/1000));
ylim([-0.1, 3.4]);
