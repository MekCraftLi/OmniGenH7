function results = run_1mhz_filter_suite()
%RUN_1MHZ_FILTER_SUITE Exercise DAC reconstruction at 1 MS/s.
%   Generates base, modulation, sinc, and sweep waveforms from 1 Hz to
%   100 kHz, applies a stable reconstruction low-pass filter, and exports
%   plot and CSV results under output_1mhz_filter_suite.

close all;
root_dir = fileparts(mfilename('fullpath'));
out_dir = fullfile(root_dir, 'output_1mhz_filter_suite');
if ~exist(out_dir, 'dir')
    mkdir(out_dir);
end

fs = 1e6;
vref_v = 3.3;
offset_v = 1.65;
peak_v = 1.50;
filter_stages = 2;
freqs_hz = [1, 10, 100, 1e3, 10e3, 50e3, 100e3];
waveforms = {'sine', 'square', 'triangle', 'sawtooth', 'am', 'fm', 'sinc', 'sweep'};

results = struct([]);
for w = 1:numel(waveforms)
    kind = waveforms{w};
    fig = figure('Visible', 'off', 'Position', [80, 40, 1500, 1800]);
    tl = tiledlayout(numel(freqs_hz), 1, 'TileSpacing', 'compact', 'Padding', 'compact');
    title(tl, sprintf('%s waveform reconstruction at 1 MS/s', upper(kind)));

    for fidx = 1:numel(freqs_hz)
        freq_hz = freqs_hz(fidx);
        [t, ideal_v, raw_v, meta] = synthesize_dac_waveform(kind, freq_hz, fs, peak_v, offset_v, vref_v);
        cutoff_hz = choose_filter_cutoff(kind, freq_hz, meta.band_upper_hz, fs);
        filtered_v = rc_reconstruction_filter(raw_v, fs, cutoff_hz, filter_stages);

        row = calculate_metrics(kind, freq_hz, fs, cutoff_hz, ideal_v, raw_v, filtered_v, meta);
        results = append_result(results, row);

        nexttile;
        plot_waveform_tile(t, raw_v, filtered_v, ideal_v, freq_hz, fs, cutoff_hz, meta);
    end

    exportgraphics(fig, fullfile(out_dir, sprintf('%s_filter_result.png', kind)), 'Resolution', 150);
    close(fig);
end

export_full_sweep(out_dir, fs, peak_v, offset_v, vref_v, filter_stages);
export_summary(out_dir, results, waveforms, freqs_hz);
writetable(struct2table(results), fullfile(out_dir, 'filter_metrics.csv'));

fprintf('1 MS/s filter suite complete.\n');
fprintf('Output directory: %s\n', out_dir);
end

function [t, ideal_v, raw_v, meta] = synthesize_dac_waveform(kind, freq_hz, fs, peak_v, offset_v, vref_v)
duration_s = choose_duration(kind, freq_hz);
n = max(32, round(duration_s * fs));
t = (0:n-1) / fs;

meta = struct( ...
    'label', sprintf('%g Hz', freq_hz), ...
    'band_upper_hz', freq_hz, ...
    'duration_s', duration_s, ...
    'samples_per_cycle', fs / max(freq_hz, eps));

switch kind
    case 'sine'
        normalized = sin(2*pi*freq_hz*t);

    case 'square'
        normalized = sign(sin(2*pi*freq_hz*t));
        normalized(normalized == 0) = 1;

    case 'triangle'
        phase = mod(freq_hz*t, 1);
        normalized = 4*abs(phase - 0.5) - 1;

    case 'sawtooth'
        phase = mod(freq_hz*t, 1);
        normalized = 2*phase - 1;

    case 'am'
        mod_hz = max(0.1, freq_hz / 20);
        envelope = 0.60 + 0.40*sin(2*pi*mod_hz*t);
        normalized = envelope .* sin(2*pi*freq_hz*t);
        meta.label = sprintf('carrier=%g Hz, mod=%g Hz', freq_hz, mod_hz);
        meta.band_upper_hz = freq_hz + mod_hz;

    case 'fm'
        mod_hz = max(0.1, freq_hz / 25);
        deviation_hz = max(0.1, min(freq_hz * 0.25, 20e3));
        beta = deviation_hz / mod_hz;
        normalized = sin(2*pi*freq_hz*t + beta*sin(2*pi*mod_hz*t));
        meta.label = sprintf('carrier=%g Hz, dev=%g Hz, mod=%g Hz', freq_hz, deviation_hz, mod_hz);
        meta.band_upper_hz = freq_hz + deviation_hz + mod_hz;

    case 'sinc'
        center_s = (n-1) / (2*fs);
        x = 2 * freq_hz * (t - center_s);
        normalized = local_sinc(x);
        meta.label = sprintf('sinc width=%g Hz', freq_hz);
        meta.band_upper_hz = 2 * freq_hz;

    case 'sweep'
        f_start = 1;
        f_stop = freq_hz;
        if f_stop <= f_start
            phase = 2*pi*f_start*t;
        else
            sweep_rate = (f_stop - f_start) / duration_s;
            phase = 2*pi*(f_start*t + 0.5*sweep_rate*t.^2);
        end
        normalized = sin(phase);
        meta.label = sprintf('sweep 1 Hz to %g Hz', f_stop);
        meta.band_upper_hz = f_stop;

    otherwise
        error('Unknown waveform: %s', kind);
end

normalized = max(-1, min(1, normalized));
ideal_v = offset_v + peak_v * normalized;
raw_v = quantize_dac_voltage(ideal_v, vref_v);
end

function duration_s = choose_duration(kind, freq_hz)
switch kind
    case 'sweep'
        duration_s = 0.20;
    case 'sinc'
        duration_s = min(max(6 / max(freq_hz, 1), 5e-3), 1.0);
    otherwise
        duration_s = min(max(5 / max(freq_hz, 1), 5e-3), 1.0);
end
end

function cutoff_hz = choose_filter_cutoff(kind, freq_hz, band_upper_hz, fs)
switch kind
    case 'sine'
        target_hz = 10.0 * freq_hz;
    case 'triangle'
        target_hz = 15.0 * freq_hz;
    case {'square', 'sawtooth'}
        target_hz = 20.0 * freq_hz;
    case {'am', 'fm'}
        target_hz = 8.0 * band_upper_hz;
    case 'sinc'
        target_hz = 5.0 * band_upper_hz;
    case 'sweep'
        target_hz = 2.0 * band_upper_hz;
    otherwise
        target_hz = 2.5 * freq_hz;
end

cutoff_hz = max(50, min(0.45 * fs, target_hz));
end

function y = rc_reconstruction_filter(x, fs, cutoff_hz, stages)
cutoff_hz = max(1, min(cutoff_hz, 0.49 * fs));
dt = 1 / fs;
tau = 1 / (2*pi*cutoff_hz);
alpha = dt / (tau + dt);

y = double(x(:)).';
for s = 1:stages
    y = rc_one_pole(y, alpha);
end

% Forward/backward pass removes group delay, matching offline simulation use.
y = fliplr(y);
for s = 1:stages
    y = rc_one_pole(y, alpha);
end
y = fliplr(y);
end

function y = rc_one_pole(x, alpha)
y = zeros(size(x));
y(1) = x(1);
for i = 2:numel(x)
    y(i) = y(i-1) + alpha * (x(i) - y(i-1));
end
end

function raw_v = quantize_dac_voltage(v, vref_v)
codes = round(max(0, min(vref_v, v)) / vref_v * 4095);
raw_v = double(codes) / 4095 * vref_v;
end

function row = calculate_metrics(kind, freq_hz, fs, cutoff_hz, ideal_v, raw_v, filtered_v, meta)
raw_err_mv = local_rms((raw_v - ideal_v) * 1000);
filtered_err_mv = local_rms((filtered_v - ideal_v) * 1000);
filter_delta_mv = local_rms((filtered_v - raw_v) * 1000);

row = struct( ...
    'waveform', string(kind), ...
    'frequency_hz', freq_hz, ...
    'sample_rate_hz', fs, ...
    'samples_per_cycle', meta.samples_per_cycle, ...
    'filter_cutoff_hz', cutoff_hz, ...
    'band_upper_hz', meta.band_upper_hz, ...
    'raw_error_rms_mv', raw_err_mv, ...
    'filtered_error_rms_mv', filtered_err_mv, ...
    'filter_delta_rms_mv', filter_delta_mv, ...
    'raw_peak_v', max(raw_v), ...
    'raw_min_v', min(raw_v), ...
    'filtered_peak_v', max(filtered_v), ...
    'filtered_min_v', min(filtered_v));
end

function results = append_result(results, row)
if isempty(results)
    results = row;
else
    results(end+1) = row; %#ok<AGROW>
end
end

function plot_waveform_tile(t, raw_v, filtered_v, ideal_v, freq_hz, fs, cutoff_hz, meta)
plot_duration_s = choose_plot_duration(t(end), freq_hz);
idx = find(t <= plot_duration_s);
idx = limit_points(idx, 5000);

[x, unit_name] = time_axis(t(idx));
stairs(x, raw_v(idx), 'Color', [0.72 0.72 0.72], 'LineWidth', 0.8);
hold on;
plot(x, ideal_v(idx), 'Color', [0.15 0.28 0.65], 'LineWidth', 0.9);
plot(x, filtered_v(idx), 'Color', [0.82 0.24 0.12], 'LineWidth', 1.1);
hold off;
grid on;
ylim([-0.1, 3.4]);
ylabel('V');
xlabel(sprintf('Time (%s)', unit_name));
title(sprintf('%s | fc=%.3g Hz | %.1f samples/cycle', ...
    meta.label, cutoff_hz, fs / max(freq_hz, eps)));
legend({'DAC ZOH', 'ideal', 'filtered'}, 'Location', 'best');
end

function plot_duration_s = choose_plot_duration(total_duration_s, freq_hz)
if freq_hz <= 10
    plot_duration_s = total_duration_s;
else
    plot_duration_s = min(total_duration_s, max(5 / freq_hz, 50e-6));
end
end

function idx = limit_points(idx, max_points)
if numel(idx) <= max_points
    return;
end
step = ceil(numel(idx) / max_points);
idx = idx(1:step:end);
end

function [x, unit_name] = time_axis(t)
max_t = max(t);
if max_t < 1e-3
    x = t * 1e6;
    unit_name = 'us';
elseif max_t < 1
    x = t * 1e3;
    unit_name = 'ms';
else
    x = t;
    unit_name = 's';
end
end

function export_full_sweep(out_dir, fs, peak_v, offset_v, vref_v, filter_stages)
[t, ideal_v, raw_v, meta] = synthesize_dac_waveform('sweep', 100e3, fs, peak_v, offset_v, vref_v);
cutoff_hz = choose_filter_cutoff('sweep', 100e3, meta.band_upper_hz, fs);
filtered_v = rc_reconstruction_filter(raw_v, fs, cutoff_hz, filter_stages);

fig = figure('Visible', 'off', 'Position', [100, 80, 1500, 850]);
tiledlayout(2, 1, 'TileSpacing', 'compact', 'Padding', 'compact');

nexttile;
idx = limit_points(1:numel(t), 10000);
plot(t(idx) * 1e3, raw_v(idx), 'Color', [0.70 0.70 0.70], 'LineWidth', 0.6);
hold on;
plot(t(idx) * 1e3, filtered_v(idx), 'Color', [0.82 0.24 0.12], 'LineWidth', 1.0);
plot(t(idx) * 1e3, ideal_v(idx), 'Color', [0.15 0.28 0.65], 'LineWidth', 0.8);
hold off;
grid on;
xlabel('Time (ms)');
ylabel('Voltage (V)');
title(sprintf('Full sweep 1 Hz to 100 kHz at 1 MS/s, fc=%.3g Hz', cutoff_hz));
legend({'DAC ZOH', 'filtered', 'ideal'}, 'Location', 'best');

nexttile;
[f_raw, mag_raw] = single_sided_spectrum(raw_v - mean(raw_v), fs);
[f_filt, mag_filt] = single_sided_spectrum(filtered_v - mean(filtered_v), fs);
semilogx(f_raw, mag_raw, 'Color', [0.70 0.70 0.70], 'LineWidth', 0.8);
hold on;
semilogx(f_filt, mag_filt, 'Color', [0.82 0.24 0.12], 'LineWidth', 1.0);
xline(100e3, 'k--', '100 kHz');
xline(fs/2, 'k:', 'Nyquist');
hold off;
grid on;
xlabel('Frequency (Hz)');
ylabel('Magnitude (dBFS)');
title('Sweep spectrum before and after reconstruction filtering');
legend({'DAC ZOH', 'filtered'}, 'Location', 'southwest');
xlim([1, fs/2]);
ylim([-120, 5]);

exportgraphics(fig, fullfile(out_dir, 'full_sweep_1hz_to_100khz.png'), 'Resolution', 150);
close(fig);
end

function export_summary(out_dir, results, waveforms, freqs_hz)
fig = figure('Visible', 'off', 'Position', [100, 80, 1250, 700]);
metric = nan(numel(waveforms), numel(freqs_hz));

for r = 1:numel(results)
    wi = find(strcmp(waveforms, char(results(r).waveform)), 1);
    fi = find(freqs_hz == results(r).frequency_hz, 1);
    metric(wi, fi) = results(r).filtered_error_rms_mv;
end

imagesc(log10(freqs_hz), 1:numel(waveforms), metric);
set(gca, 'YTick', 1:numel(waveforms), 'YTickLabel', waveforms);
set(gca, 'XTick', log10(freqs_hz), 'XTickLabel', compose('%g', freqs_hz));
colorbar;
grid on;
xlabel('Frequency (Hz)');
ylabel('Waveform');
title('Filtered RMS error versus ideal waveform (mV)');

for wi = 1:numel(waveforms)
    for fi = 1:numel(freqs_hz)
        text(log10(freqs_hz(fi)), wi, sprintf('%.1f', metric(wi, fi)), ...
            'HorizontalAlignment', 'center', 'Color', 'w', 'FontWeight', 'bold');
    end
end

exportgraphics(fig, fullfile(out_dir, 'filter_error_summary.png'), 'Resolution', 150);
close(fig);
end

function [freq_hz, mag_db] = single_sided_spectrum(x, fs)
n = numel(x);
window = 0.5 - 0.5*cos(2*pi*(0:n-1)/(n-1));
xw = x(:).' .* window;
y = fft(xw);
p = abs(y(1:floor(n/2)+1));
p = p / max(p + eps);
mag_db = 20*log10(p + eps);
freq_hz = (0:numel(p)-1) * fs / n;
freq_hz(1) = max(freq_hz(2), 1e-9);
end

function y = local_sinc(x)
y = ones(size(x));
nz = abs(x) > eps;
y(nz) = sin(pi*x(nz)) ./ (pi*x(nz));
end

function value = local_rms(x)
value = sqrt(mean(double(x(:)).^2));
end
