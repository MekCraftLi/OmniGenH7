function results = simulate_recommended_filters()
%SIMULATE_RECOMMENDED_FILTERS Simulate the recommended analog filter plan.
%   The filters are modeled at 1 MS/s as reconstruction low-pass stages:
%   - Bypass/Wide: output buffer only
%   - Low:  4th-order active Butterworth, fc = 25 kHz
%   - Mid:  4th-order active Butterworth, fc = 125 kHz
%   - High: 4th-order active Butterworth, fc = 225 kHz
%
%   Output files are written to output_filter_recommendations.

close all;
root_dir = fileparts(mfilename('fullpath'));
out_dir = fullfile(root_dir, 'output_filter_recommendations');
if ~exist(out_dir, 'dir')
    mkdir(out_dir);
end

fs = 1e6;
vref_v = 3.3;
offset_v = 1.65;
peak_v = 1.45;
filters = filter_bank();

recommended = [
    make_case('sine',      'Sine 50 kHz',          'high', 50e3, 0, 0, 0)
    make_case('am',        'AM 30 kHz carrier',    'mid',  30e3, 1.5e3, 0, 0)
    make_case('fm',        'FM 30 kHz carrier',    'mid',  30e3, 1.2e3, 7.5e3, 0)
    make_case('triangle',  'Triangle 20 kHz',      'mid',  20e3, 0, 0, 0)
    make_case('square',    'Square 10 kHz',        'mid',  10e3, 0, 0, 0)
    make_case('sawtooth',  'Sawtooth 10 kHz',      'mid',  10e3, 0, 0, 0)
    make_case('sinc',      'Sinc 100 kHz BW',      'high', 100e3, 0, 0, 0)
    make_case('sweep',     'Sweep 1 Hz to 50 kHz', 'mid',  50e3, 0, 0, 0)
    make_case('arbitrary', 'Arbitrary 50 kHz BW',  'mid',  50e3, 0, 0, 0)
];

extension = [
    make_case('sine',     'Sine 100 kHz',          'high', 100e3, 0, 0, 0)
    make_case('am',       'AM 50 kHz carrier',    'high', 50e3, 2.5e3, 0, 0)
    make_case('fm',       'FM 50 kHz carrier',    'high', 50e3, 2.0e3, 10e3, 0)
    make_case('triangle', 'Triangle 50 kHz',      'high', 50e3, 0, 0, 0)
    make_case('sweep',    'Sweep 1 Hz to 100 kHz','high', 100e3, 0, 0, 0)
];

export_filter_responses(out_dir, fs, filters);
results = export_case_grid(out_dir, fs, vref_v, offset_v, peak_v, filters, recommended, ...
    'recommended_waveform_filter_results.png', 'Recommended Filter Results');
limit_results = export_case_grid(out_dir, fs, vref_v, offset_v, peak_v, filters, extension, ...
    'extension_limit_filter_results.png', 'Extension Limit Cases');
export_active_passive_comparison(out_dir, fs, vref_v, offset_v, peak_v);

results = [results; limit_results];
writetable(results, fullfile(out_dir, 'recommended_filter_metrics.csv'));

fprintf('Recommended filter simulation complete.\n');
fprintf('Output directory: %s\n', out_dir);
end

function c = make_case(kind, label, filter_name, freq_hz, mod_hz, deviation_hz, bandwidth_hz)
c = struct( ...
    'kind', kind, ...
    'label', label, ...
    'filter_name', filter_name, ...
    'freq_hz', freq_hz, ...
    'mod_hz', mod_hz, ...
    'deviation_hz', deviation_hz, ...
    'bandwidth_hz', bandwidth_hz);
end

function filters = filter_bank()
filters = containers.Map();
filters('bypass') = struct('label', 'Bypass / Wide buffer', 'order', 0, 'fc_hz', inf, 'type', 'bypass');
filters('low') = struct('label', 'Low active LPF 25 kHz', 'order', 4, 'fc_hz', 25e3, 'type', 'butter');
filters('mid') = struct('label', 'Mid active LPF 125 kHz', 'order', 4, 'fc_hz', 125e3, 'type', 'butter');
filters('high') = struct('label', 'High active LPF 225 kHz', 'order', 4, 'fc_hz', 225e3, 'type', 'butter');
end

function rows = export_case_grid(out_dir, fs, vref_v, offset_v, peak_v, filters, cases, file_name, title_text)
fig = figure('Visible', 'off', 'Position', [60, 40, 1600, 1900]);
tl = tiledlayout(numel(cases), 1, 'TileSpacing', 'compact', 'Padding', 'compact');
title(tl, sprintf('%s at 1 MS/s', title_text));

rows = table();
for i = 1:numel(cases)
    c = cases(i);
    [t, ideal_v, raw_v, meta] = synthesize_case(c, fs, vref_v, offset_v, peak_v);
    filter_cfg = filters(c.filter_name);
    filtered_v = apply_filter(raw_v, fs, filter_cfg);

    nexttile;
    plot_case(t, ideal_v, raw_v, filtered_v, c, filter_cfg, meta);

    row = case_metrics(c, fs, meta, filter_cfg, ideal_v, raw_v, filtered_v);
    rows = [rows; struct2table(row)]; %#ok<AGROW>
end

exportgraphics(fig, fullfile(out_dir, file_name), 'Resolution', 150);
close(fig);
end

function [t, ideal_v, raw_v, meta] = synthesize_case(c, fs, vref_v, offset_v, peak_v)
duration_s = choose_duration(c.kind, c.freq_hz);
n = max(128, round(duration_s * fs));
t = (0:n-1) / fs;

meta = struct( ...
    'band_upper_hz', c.freq_hz, ...
    'samples_per_cycle', fs / max(c.freq_hz, eps));

switch c.kind
    case 'sine'
        x = sin(2*pi*c.freq_hz*t);

    case 'am'
        envelope = 0.60 + 0.40*sin(2*pi*c.mod_hz*t);
        x = envelope .* sin(2*pi*c.freq_hz*t);
        meta.band_upper_hz = c.freq_hz + c.mod_hz;

    case 'fm'
        beta = c.deviation_hz / c.mod_hz;
        x = sin(2*pi*c.freq_hz*t + beta*sin(2*pi*c.mod_hz*t));
        meta.band_upper_hz = c.freq_hz + c.deviation_hz + c.mod_hz;

    case 'triangle'
        phase = mod(c.freq_hz*t, 1);
        x = 4*abs(phase - 0.5) - 1;

    case 'square'
        x = sign(sin(2*pi*c.freq_hz*t));
        x(x == 0) = 1;

    case 'sawtooth'
        phase = mod(c.freq_hz*t, 1);
        x = 2*phase - 1;

    case 'sinc'
        center_s = (n-1) / (2*fs);
        x = local_sinc(2*c.freq_hz*(t - center_s));
        meta.band_upper_hz = c.freq_hz;

    case 'sweep'
        f_start = 1;
        f_stop = c.freq_hz;
        k = (f_stop - f_start) / duration_s;
        phase = 2*pi*(f_start*t + 0.5*k*t.^2);
        x = sin(phase);
        meta.band_upper_hz = f_stop;

    case 'arbitrary'
        x = 0.50*sin(2*pi*3e3*t) + 0.30*sin(2*pi*17e3*t + 0.4) + 0.20*sin(2*pi*50e3*t + 1.1);
        x = x / max(abs(x));
        meta.band_upper_hz = c.freq_hz;

    otherwise
        error('Unknown waveform: %s', c.kind);
end

x = max(-1, min(1, x));
ideal_v = offset_v + peak_v * x;
raw_v = quantize_dac_voltage(ideal_v, vref_v);
end

function duration_s = choose_duration(kind, freq_hz)
switch kind
    case 'sweep'
        duration_s = 0.10;
    case 'sinc'
        duration_s = 8e-3;
    otherwise
        duration_s = min(max(8 / max(freq_hz, 1), 4e-3), 0.10);
end
end

function y = apply_filter(x, fs, filter_cfg)
switch filter_cfg.type
    case 'bypass'
        y = double(x(:)).';
    case 'butter'
        wn = min(0.98, filter_cfg.fc_hz / (fs / 2));
        [b, a] = butter(filter_cfg.order, wn, 'low');
        y = filtfilt(b, a, double(x(:)).');
    otherwise
        error('Unknown filter type: %s', filter_cfg.type);
end
end

function y = apply_passive_rc(x, fs, fc_hz)
dt = 1 / fs;
tau = 1 / (2*pi*fc_hz);
alpha = dt / (tau + dt);
y = rc_one_pole(double(x(:)).', alpha);
y = fliplr(y);
y = rc_one_pole(y, alpha);
y = fliplr(y);
end

function y = rc_one_pole(x, alpha)
y = zeros(size(x));
y(1) = x(1);
for i = 2:numel(x)
    y(i) = y(i-1) + alpha * (x(i) - y(i-1));
end
end

function plot_case(t, ideal_v, raw_v, filtered_v, c, filter_cfg, meta)
plot_duration_s = min(t(end), max(5 / max(c.freq_hz, 1), 80e-6));
if strcmp(c.kind, 'sweep') || strcmp(c.kind, 'sinc')
    plot_duration_s = t(end);
end
idx = find(t <= plot_duration_s);
idx = limit_points(idx, 5000);
[x_axis, unit_name] = time_axis(t(idx));

stairs(x_axis, raw_v(idx), 'Color', [0.70 0.70 0.70], 'LineWidth', 0.7);
hold on;
plot(x_axis, ideal_v(idx), 'Color', [0.12 0.25 0.65], 'LineWidth', 0.85);
plot(x_axis, filtered_v(idx), 'Color', [0.82 0.22 0.12], 'LineWidth', 1.1);
hold off;
grid on;
ylim([-0.1, 3.4]);
xlabel(sprintf('Time (%s)', unit_name));
ylabel('V');
title(sprintf('%s | %s | fc=%s | band=%.3g Hz | %.1f pts/cycle', ...
    c.label, filter_cfg.label, cutoff_label(filter_cfg), meta.band_upper_hz, meta.samples_per_cycle));
legend({'DAC ZOH', 'ideal', 'filtered'}, 'Location', 'best');
end

function text_value = cutoff_label(filter_cfg)
if isinf(filter_cfg.fc_hz)
    text_value = 'bypass';
else
    text_value = sprintf('%.0f kHz', filter_cfg.fc_hz / 1e3);
end
end

function row = case_metrics(c, fs, meta, filter_cfg, ideal_v, raw_v, filtered_v)
row = struct( ...
    'case_label', string(c.label), ...
    'waveform', string(c.kind), ...
    'recommended_filter', string(filter_cfg.label), ...
    'sample_rate_hz', fs, ...
    'frequency_hz', c.freq_hz, ...
    'band_upper_hz', meta.band_upper_hz, ...
    'samples_per_cycle', meta.samples_per_cycle, ...
    'filter_order', filter_cfg.order, ...
    'filter_cutoff_hz', filter_cfg.fc_hz, ...
    'raw_error_rms_mv', local_rms((raw_v - ideal_v) * 1000), ...
    'filtered_error_rms_mv', local_rms((filtered_v - ideal_v) * 1000), ...
    'raw_vpp', max(raw_v) - min(raw_v), ...
    'filtered_vpp', max(filtered_v) - min(filtered_v));
end

function export_filter_responses(out_dir, fs, filters)
fig = figure('Visible', 'off', 'Position', [100, 80, 1350, 850]);
tiledlayout(2, 1, 'TileSpacing', 'compact', 'Padding', 'compact');

nexttile;
hold on;
names = {'low', 'mid', 'high'};
colors = [0.15 0.35 0.75; 0.10 0.55 0.35; 0.82 0.22 0.12];
for i = 1:numel(names)
    fcfg = filters(names{i});
    [b, a] = butter(fcfg.order, fcfg.fc_hz/(fs/2), 'low');
    [h, w] = freqz(b, a, 4096, fs);
    semilogx(w, 20*log10(abs(h) + eps), 'LineWidth', 1.4, 'Color', colors(i,:));
end
yline(-3, 'k--', '-3 dB');
xline(100e3, 'k:', '100 kHz');
xline(fs/2, 'k:', 'Nyquist');
hold off;
grid on;
xlabel('Frequency (Hz)');
ylabel('Magnitude (dB)');
title('Active 4th-order Butterworth filter bank response');
legend({'Low 25 kHz', 'Mid 125 kHz', 'High 225 kHz'}, 'Location', 'southwest');
xlim([1e3, fs/2]);
ylim([-90, 5]);

nexttile;
hold on;
for i = 1:numel(names)
    fcfg = filters(names{i});
    [b, a] = butter(fcfg.order, fcfg.fc_hz/(fs/2), 'low');
    [gd, w] = grpdelay(b, a, 1024, fs);
    semilogx(w, gd / fs * 1e6, 'LineWidth', 1.2, 'Color', colors(i,:));
end
hold off;
grid on;
xlabel('Frequency (Hz)');
ylabel('Group delay (us)');
title('Filter bank group delay estimate');
legend({'Low 25 kHz', 'Mid 125 kHz', 'High 225 kHz'}, 'Location', 'northeast');
xlim([1e3, fs/2]);

exportgraphics(fig, fullfile(out_dir, 'filter_bank_response.png'), 'Resolution', 150);
close(fig);
end

function export_active_passive_comparison(out_dir, fs, vref_v, offset_v, peak_v)
cases = [
    make_case('sine',     'Sine 100 kHz',     'high', 100e3, 0, 0, 0)
    make_case('triangle', 'Triangle 20 kHz',  'mid',  20e3, 0, 0, 0)
    make_case('am',       'AM 30 kHz carrier','mid',  30e3, 1.5e3, 0, 0)
];

fig = figure('Visible', 'off', 'Position', [80, 60, 1500, 1100]);
tl = tiledlayout(numel(cases), 1, 'TileSpacing', 'compact', 'Padding', 'compact');
title(tl, 'Passive RC versus active 4th-order low-pass');

for i = 1:numel(cases)
    c = cases(i);
    [t, ideal_v, raw_v, meta] = synthesize_case(c, fs, vref_v, offset_v, peak_v);
    fc_hz = 225e3;
    if strcmp(c.filter_name, 'mid')
        fc_hz = 125e3;
    end
    [b, a] = butter(4, fc_hz/(fs/2), 'low');
    active_v = filtfilt(b, a, raw_v);
    passive_v = apply_passive_rc(raw_v, fs, fc_hz);

    nexttile;
    plot_duration_s = min(t(end), max(5 / max(c.freq_hz, 1), 80e-6));
    idx = limit_points(find(t <= plot_duration_s), 5000);
    [x_axis, unit_name] = time_axis(t(idx));
    stairs(x_axis, raw_v(idx), 'Color', [0.72 0.72 0.72], 'LineWidth', 0.6);
    hold on;
    plot(x_axis, ideal_v(idx), 'Color', [0.12 0.25 0.65], 'LineWidth', 0.85);
    plot(x_axis, passive_v(idx), 'Color', [0.10 0.55 0.35], 'LineWidth', 1.0);
    plot(x_axis, active_v(idx), 'Color', [0.82 0.22 0.12], 'LineWidth', 1.0);
    hold off;
    grid on;
    ylim([-0.1, 3.4]);
    xlabel(sprintf('Time (%s)', unit_name));
    ylabel('V');
    title(sprintf('%s | fc=%.0f kHz | band=%.3g Hz', c.label, fc_hz/1e3, meta.band_upper_hz));
    legend({'DAC ZOH', 'ideal', 'passive RC', 'active 4th-order'}, 'Location', 'best');
end

exportgraphics(fig, fullfile(out_dir, 'active_vs_passive_comparison.png'), 'Resolution', 150);
close(fig);
end

function raw_v = quantize_dac_voltage(v, vref_v)
codes = round(max(0, min(vref_v, v)) / vref_v * 4095);
raw_v = double(codes) / 4095 * vref_v;
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

function y = local_sinc(x)
y = ones(size(x));
nz = abs(x) > eps;
y(nz) = sin(pi*x(nz)) ./ (pi*x(nz));
end

function value = local_rms(x)
value = sqrt(mean(double(x(:)).^2));
end
