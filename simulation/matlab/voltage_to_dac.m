function dac_val = voltage_to_dac(voltage_mv, vref_mv)
% Convert mV to 12-bit DAC value (matches C++ firmware).
if nargin < 2, vref_mv = 3300; end
if voltage_mv < 0
    dac_val = 0;
elseif voltage_mv >= vref_mv
    dac_val = 4095;
else
    dac_val = uint16(floor(voltage_mv * 4095 / vref_mv));
end
end
