from SCons.Script import (
    ARGUMENTS,
    COMMAND_LINE_TARGETS,
    DefaultEnvironment,
)

env = DefaultEnvironment()

try:
    import hid
except ImportError:
    env.Execute(
        env.VerboseAction(
            '$PYTHONEXE -m pip install "hidapi"',
            "Installing hidapi",
        )
    )

    import hid

def pre_program_action(source, target, env):
    RESET_REPORT_ID = 0x3
    USB_VID = 0x1209
    USB_PID = 0xB196

    usb_device = hid.device()
    usb_device.open(USB_VID, USB_PID)

    print("Manufacturer: %s" % usb_device.get_manufacturer_string())
    print("Product: %s" % usb_device.get_product_string())
    print("Serial No: %s" % usb_device.get_serial_number_string())

    usb_device.write([RESET_REPORT_ID])

env.AddPreAction("$PROGPATH", pre_program_action)