function samples = waveform_generator(profile, count)
%WAVEFORM_GENERATOR Generate DAC samples using the firmware-matched helpers.
%   samples = waveform_generator(profile, count) returns uint16 DAC codes.
%   Calling waveform_generator() with no arguments plots a default sine wave.

if nargin < 1 || isempty(profile)
    profile = struct( ...
        'kind', 'sine', ...
        'amplitude_mv', 3300, ...
        'offset_mv', 1650, ...
        'duty', 500);
end

if nargin < 2 || isempty(count)
    count = 256;
end

if ~isfield(profile, 'duty')
    profile.duty = 500;
end

samples = generate_waveform(count, profile);

if nargout == 0
    stairs(0:count-1, double(samples), 'LineWidth', 1.2);
    grid on;
    xlabel('Sample index');
    ylabel('DAC code');
    title(sprintf('Generated %s waveform (%d samples)', profile.kind, count));
    ylim([-64, 4160]);
    clear samples;
end
end
