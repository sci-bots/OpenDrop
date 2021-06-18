import serial
import numpy as np
import struct


class SerialProxy():
    def __init__(self, port):
        '''
        Initialize a SerialProxy object.

        Parameters
        ----------
        port : string
            Serial port name (e.g., 'COM1' or '/dev/ttyUSB0')
        '''
        self._serial = serial.Serial(port, 115200)
        
        (self.manufacturer, self.model, self.serial_number,
             self.software_version) = self.identify().split(',')
        
    def __del__(self):
        # Release the serial port
        self._serial.close()

    def identify(self):
        # Return a string that uniquely identifies the OpenDrop.
        # The string is of the form "GaudiLabs,<model>,<serial number>,<software revision>" .
        self._serial.write(b"*IDN?\n")
        return self._serial.readline().strip().decode()
    
    @property
    def voltage(self):
        '''
        Get the voltage.

        Returns
        ----------
        value : float
            RMS voltage.
        '''
        self._serial.write(b"VOLTAGE?\n")
        return float(self._serial.readline().strip())
    
    @voltage.setter
    def voltage(self, value):
        '''
        Set the voltage.

        Parameters
        ----------
        value : float
            RMS voltage.
        '''
        self._serial.write(b"VOLTAGE %f\n" % value)
    
    def set_state_of_channels(self, state):
        '''
        Set state of channels on device using state bytes.

        See also: `state_of_channels` (get)

        Parameters
        ----------
        states : list or np.array
            0 or 1 for each channel (size must be equal to the total
            number of channels).
        '''
        assert(len(state) == len(self.state_of_channels))
        
        # Cast the incoming state variable as a numpy byte array
        state_of_channels = np.array(state).astype(np.uint8)

        # Pack the channel array into a hex formatted string
        self._serial.write(b'CHAN:STAT ' + 
                            ''.join(['%02x' % x for x in np.packbits(
                                state_of_channels)]).encode() +
                            b'\n')
                            
    @property
    def state_of_channels(self):
        '''
        Get the state of the channels on device.

        See also: `set_state_of_channels`

        Returns
        ----------
        states : np.array
            0 or 1 for each channel.
        '''
        self._serial.write(b'CHAN:STAT?\n')
        state_str = self._serial.readline().strip().decode()
        
        # Unpack the channel state array from a hex formatted string into an
        # array.
        return np.unpackbits(np.array(
            struct.unpack('B'*16,bytearray.fromhex(state_str)), np.uint8))
