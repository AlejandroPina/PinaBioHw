from collections import defaultdict
from skidl import Pin, Part, Alias, SchLib, SKIDL, TEMPLATE

from skidl.pin import pin_types

SKIDL_lib_version = '0.0.1'

generate_kicad = SchLib(tool=SKIDL).add_parts(*[
        Part(**{ 'name':'XIAO_ESP32S3', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'XIAO_ESP32S3'}), 'ref_prefix':'U', 'fplist':['Pina:XIAO_ESP32S3_Socket'], 'footprint':'Pina:XIAO_ESP32S3_Socket', 'keywords':'Seeed XIAO ESP32-S3', 'description':'', 'datasheet':'https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/', 'pins':[
            Pin(num='1',name='D0/A0',func=pin_types.BIDIR,unit=1),
            Pin(num='2',name='D1',func=pin_types.BIDIR,unit=1),
            Pin(num='3',name='D2',func=pin_types.BIDIR,unit=1),
            Pin(num='4',name='D3/~SLEEP',func=pin_types.BIDIR,unit=1),
            Pin(num='5',name='D4/SDA',func=pin_types.BIDIR,unit=1),
            Pin(num='6',name='D5/SCL',func=pin_types.BIDIR,unit=1),
            Pin(num='7',name='D6/TX',func=pin_types.BIDIR,unit=1),
            Pin(num='8',name='D7/RX',func=pin_types.BIDIR,unit=1),
            Pin(num='9',name='D8',func=pin_types.BIDIR,unit=1),
            Pin(num='10',name='D9',func=pin_types.BIDIR,unit=1),
            Pin(num='11',name='D10',func=pin_types.BIDIR,unit=1),
            Pin(num='12',name='3V3',func=pin_types.PWROUT,unit=1),
            Pin(num='13',name='GND',func=pin_types.PWRIN,unit=1),
            Pin(num='14',name='5V',func=pin_types.PWRIN,unit=1),
            Pin(num='15',name='BAT+',func=pin_types.PWRIN,unit=1)], 'unit_defs':[] }),
        Part(**{ 'name':'Conn_01x02', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'Conn_01x02'}), 'ref_prefix':'J', 'fplist':[''], 'footprint':'Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal', 'keywords':'connector', 'description':'Generic connector, single row, 01x02, script generated (kicad-library-utils/schlib/autogen/connector/)', 'datasheet':'~', 'pins':[
            Pin(num='1',name='Pin_1',func=pin_types.PASSIVE,unit=1),
            Pin(num='2',name='Pin_2',func=pin_types.PASSIVE,unit=1)], 'unit_defs':[] }),
        Part(**{ 'name':'SW_DPDT_ONOFF', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'SW_DPDT_ONOFF'}), 'ref_prefix':'SW', 'fplist':['Button_Switch_THT:SW_CK_JS202011CQN_DPDT_Straight'], 'footprint':'Button_Switch_THT:SW_CK_JS202011CQN_DPDT_Straight', 'keywords':'', 'description':'', 'datasheet':'https://www.ckswitches.com/media/1422/js.pdf', 'pins':[
            Pin(num='1',name='A_ON',func=pin_types.PASSIVE,unit=1),
            Pin(num='2',name='A_COM',func=pin_types.PASSIVE,unit=1),
            Pin(num='3',name='A_OFF',func=pin_types.PASSIVE,unit=1),
            Pin(num='4',name='B_OFF',func=pin_types.PASSIVE,unit=1),
            Pin(num='5',name='B_COM',func=pin_types.PASSIVE,unit=1),
            Pin(num='6',name='B_ON',func=pin_types.PASSIVE,unit=1)], 'unit_defs':[] }),
        Part(**{ 'name':'ADP150-3.3', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'ADP150-3.3'}), 'ref_prefix':'U', 'fplist':['Package_TO_SOT_SMD:SOT-23-5'], 'footprint':'Package_TO_SOT_SMD:SOT-23-5', 'keywords':'LDO 3.3V analog', 'description':'', 'datasheet':'https://www.analog.com/media/en/technical-documentation/data-sheets/ADP150.pdf', 'pins':[
            Pin(num='1',name='VIN',func=pin_types.PWRIN,unit=1),
            Pin(num='2',name='GND',func=pin_types.PWRIN,unit=1),
            Pin(num='3',name='EN',func=pin_types.INPUT,unit=1),
            Pin(num='4',name='NC',func=pin_types.NOCONNECT,unit=1),
            Pin(num='5',name='VOUT',func=pin_types.PWROUT,unit=1)], 'unit_defs':[] }),
        Part(**{ 'name':'C', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'C'}), 'ref_prefix':'C', 'fplist':[''], 'footprint':'Capacitor_SMD:C_1206_3216Metric', 'keywords':'cap capacitor', 'description':'Unpolarized capacitor', 'datasheet':'~', 'pins':[
            Pin(num='1',name='~',func=pin_types.PASSIVE,unit=1),
            Pin(num='2',name='~',func=pin_types.PASSIVE,unit=1)], 'unit_defs':[] }),
        Part(**{ 'name':'FerriteBead', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'FerriteBead'}), 'ref_prefix':'FB', 'fplist':[''], 'footprint':'Inductor_SMD:L_0805_2012Metric', 'keywords':'L ferrite bead inductor filter', 'description':'Ferrite bead', 'datasheet':'~', 'pins':[
            Pin(num='1',name='~',func=pin_types.PASSIVE,unit=1),
            Pin(num='2',name='~',func=pin_types.PASSIVE,unit=1)], 'unit_defs':[] }),
        Part(**{ 'name':'NetTie_2', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'NetTie_2'}), 'ref_prefix':'NT', 'fplist':[''], 'footprint':'NetTie:NetTie-2_SMD_Pad2.0mm', 'keywords':'net tie short', 'description':'Net tie, 2 pins', 'datasheet':'~', 'pins':[
            Pin(num='1',name='1',func=pin_types.PASSIVE,unit=1),
            Pin(num='2',name='2',func=pin_types.PASSIVE,unit=1)], 'unit_defs':[] }),
        Part(**{ 'name':'R', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'R'}), 'ref_prefix':'R', 'fplist':[''], 'footprint':'Resistor_SMD:R_1206_3216Metric', 'keywords':'R res resistor', 'description':'Resistor', 'datasheet':'~', 'pins':[
            Pin(num='1',name='~',func=pin_types.PASSIVE,unit=1),
            Pin(num='2',name='~',func=pin_types.PASSIVE,unit=1)], 'unit_defs':[] }),
        Part(**{ 'name':'MCP6004', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'MCP6004'}), 'ref_prefix':'U', 'fplist':['', ''], 'footprint':'Package_SO:SOIC-14_3.9x8.7mm_P1.27mm', 'keywords':'quad opamp', 'description':'1MHz, Low-Power Op Amp, DIP-14/SOIC-14/TSSOP-14', 'datasheet':'http://ww1.microchip.com/downloads/en/DeviceDoc/21733j.pdf', 'pins':[
            Pin(num='3',name='+',func=pin_types.INPUT,unit=1),
            Pin(num='2',name='-',func=pin_types.INPUT,unit=1),
            Pin(num='1',name='~',func=pin_types.OUTPUT,unit=1),
            Pin(num='5',name='+',func=pin_types.INPUT,unit=2),
            Pin(num='6',name='-',func=pin_types.INPUT,unit=2),
            Pin(num='7',name='~',func=pin_types.OUTPUT,unit=2),
            Pin(num='10',name='+',func=pin_types.INPUT,unit=3),
            Pin(num='9',name='-',func=pin_types.INPUT,unit=3),
            Pin(num='8',name='~',func=pin_types.OUTPUT,unit=3),
            Pin(num='12',name='+',func=pin_types.INPUT,unit=4),
            Pin(num='13',name='-',func=pin_types.INPUT,unit=4),
            Pin(num='14',name='~',func=pin_types.OUTPUT,unit=4),
            Pin(num='4',name='V+',func=pin_types.PWRIN,unit=5),
            Pin(num='11',name='V-',func=pin_types.PWRIN,unit=5)], 'unit_defs':[{'label': 'uA', 'num': 1, 'pin_nums': ['2', '3', '1']},{'label': 'uB', 'num': 2, 'pin_nums': ['6', '5', '7']},{'label': 'uC', 'num': 3, 'pin_nums': ['10', '8', '9']},{'label': 'uD', 'num': 4, 'pin_nums': ['12', '14', '13']},{'label': 'uE', 'num': 5, 'pin_nums': ['4', '11']}] }),
        Part(**{ 'name':'ADS1115IDGS', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'ADS1115IDGS'}), 'ref_prefix':'U', 'fplist':['Package_SO:TSSOP-10_3x3mm_P0.5mm', 'Package_SO:TSSOP-10_3x3mm_P0.5mm'], 'footprint':'Package_SO:TSSOP-10_3x3mm_P0.5mm', 'keywords':'16 bit 4 channel I2C ADC', 'description':'Ultra-Small, Low-Power, I2C-Compatible, 860-SPS, 16-Bit ADCs With Internal Reference, Oscillator, and Programmable Comparator, VSSOP-10', 'datasheet':'http://www.ti.com/lit/ds/symlink/ads1113.pdf', 'pins':[
            Pin(num='4',name='AIN0',func=pin_types.INPUT,unit=1),
            Pin(num='5',name='AIN1',func=pin_types.INPUT,unit=1),
            Pin(num='6',name='AIN2',func=pin_types.INPUT,unit=1),
            Pin(num='7',name='AIN3',func=pin_types.INPUT,unit=1),
            Pin(num='8',name='VDD',func=pin_types.PWRIN,unit=1),
            Pin(num='3',name='GND',func=pin_types.PWRIN,unit=1),
            Pin(num='2',name='ALERT/RDY',func=pin_types.OUTPUT,unit=1),
            Pin(num='10',name='SCL',func=pin_types.INPUT,unit=1),
            Pin(num='9',name='SDA',func=pin_types.BIDIR,unit=1),
            Pin(num='1',name='ADDR',func=pin_types.INPUT,unit=1)], 'unit_defs':[] }),
        Part(**{ 'name':'D_TVS', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'D_TVS'}), 'ref_prefix':'D', 'fplist':[''], 'footprint':'Diode_SMD:D_SOD-323', 'keywords':'diode TVS thyrector', 'description':'Bidirectional transient-voltage-suppression diode', 'datasheet':'~', 'pins':[
            Pin(num='1',name='A1',func=pin_types.PASSIVE,unit=1),
            Pin(num='2',name='A2',func=pin_types.PASSIVE,unit=1)], 'unit_defs':[] }),
        Part(**{ 'name':'Conn_01x04', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'Conn_01x04'}), 'ref_prefix':'J', 'fplist':[''], 'footprint':'Pina:SensorCable_4P_Anchor', 'keywords':'connector', 'description':'Generic connector, single row, 01x04, script generated (kicad-library-utils/schlib/autogen/connector/)', 'datasheet':'~', 'pins':[
            Pin(num='1',name='Pin_1',func=pin_types.PASSIVE,unit=1),
            Pin(num='2',name='Pin_2',func=pin_types.PASSIVE,unit=1),
            Pin(num='3',name='Pin_3',func=pin_types.PASSIVE,unit=1),
            Pin(num='4',name='Pin_4',func=pin_types.PASSIVE,unit=1)], 'unit_defs':[] }),
        Part(**{ 'name':'Conn_01x06', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'Conn_01x06'}), 'ref_prefix':'J', 'fplist':[''], 'footprint':'Pina:SensorCable_6P_Anchor', 'keywords':'connector', 'description':'Generic connector, single row, 01x06, script generated (kicad-library-utils/schlib/autogen/connector/)', 'datasheet':'~', 'pins':[
            Pin(num='1',name='Pin_1',func=pin_types.PASSIVE,unit=1),
            Pin(num='2',name='Pin_2',func=pin_types.PASSIVE,unit=1),
            Pin(num='3',name='Pin_3',func=pin_types.PASSIVE,unit=1),
            Pin(num='4',name='Pin_4',func=pin_types.PASSIVE,unit=1),
            Pin(num='5',name='Pin_5',func=pin_types.PASSIVE,unit=1),
            Pin(num='6',name='Pin_6',func=pin_types.PASSIVE,unit=1)], 'unit_defs':[] }),
        Part(**{ 'name':'MountingHole', 'dest':TEMPLATE, 'tool':SKIDL, 'aliases':Alias({'MountingHole'}), 'ref_prefix':'H', 'fplist':[''], 'footprint':'MountingHole:MountingHole_3.2mm_M3', 'keywords':'mounting hole', 'description':'Mounting Hole without connection', 'datasheet':'~' })])