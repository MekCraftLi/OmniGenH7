function samples = generate_square(count, amplitude, offset, duty)
% Square wave with duty cycle in permille (0-1000).
threshold = floor(count * duty / 1000);

high_level = int32(offset) + int32(amplitude) / 2;
low_level  = int32(offset) - int32(amplitude) / 2;
if high_level > 4095, high_level = 4095; end
if low_level < 0,    low_level = 0;     end

samples = zeros(1, count, 'uint16');
for i = 1:count
    if i <= threshold
        samples(i) = uint16(high_level);
    else
        samples(i) = uint16(low_level);
    end
end
end
