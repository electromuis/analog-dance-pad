Import("env")

lufa_list = [
    "HIDParser.c",
    "Device_AVR8.c",
    "EndpointStream_AVR8.c",
    "Endpoint_AVR8.c",
    "Host_AVR8.c",
    "PipeStream_AVR8.c",
    "Pipe_AVR8.c",
    "USBController_AVR8.c",
    "USBInterrupt_AVR8.c",
    "ConfigDescriptors.c",
    "DeviceStandardReq.c",
    "Events.c",
    "HostStandardReq.c",
    "USBTask.c",
    "AudioClassDevice.c",
    "CCIDClassDevice.c",
    "CDCClassDevice.c",
    "HIDClassDevice.c",
    "MassStorageClassDevice.c",
    "MIDIClassDevice.c",
    "PrinterClassDevice.c",
    "RNDISClassDevice.c",
    "AndroidAccessoryClassHost.c",
    "AudioClassHost.c",
    "CDCClassHost.c",
    "HIDClassHost.c",
    "MassStorageClassHost.c",
    "MIDIClassHost.c",
    "PrinterClassHost.c",
    "RNDISClassHost.c",
    "StillImageClassHost.c"
]

def skip_from_build(node):
    """
    `node.name` - a name of File System Node
    `node.get_path()` - a relative path
    `node.get_abspath()` - an absolute path
     to ignore file from a build process, just return None
    """
    if "lufa" in node.get_path():
        list_match = [x for x in lufa_list if x in node.get_path()]
        if len(list_match) == 0:
            return None
        
        old_cflags = env["CCFLAGS"]
        new_cflags = [opt for opt in old_cflags]
        new_cflags += ["-D __AVR_ATmega32U4__", "-D ARCH=ARCH_AVR8", "-D USE_LUFA_CONFIG_HEADER"]
        
        return env.Object(
            node,
            CCFLAGS=new_cflags
        )

    return node

# Register callback
env.AddBuildMiddleware(skip_from_build, "*")
