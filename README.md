# Take Recorder Auto-Trigger on Vicon Shogun Packet Plugin for UE4/UE5

This plugin is designed to trigger Unreal Engine Take Recorder when a Vicon packet is received, auto-name the take based on the vicon packet, and cut when Vicon cuts.  This was built for a project back in 2020-2021 and has not been tried since.

This is a UNFINISHED, UNSUPPORTED plugin that can be built on or tried, with no promises it will work.  Last time I tried it, in 4.26, it triggered and recorded perfectly, but could trigger a crash if you were doing other stuff when it tried to turnover. Iv compiled it in UE5 and it compiles fine, but I dont have a Vicon Shogun license to test if it works anymore.

It works by opening a UDP port on engine bootup, listens to the Vicon port (default 30), and parses the XML packet via the "EasyXMLParser" which is NOT mine. Please visit:
https://github.com/ayumax/EasyXMLParser for the latest and greatest version.

I hold no responsibility for how it works for you and cannot offer any support.

The record feature can be enabled/disabled in the project settings so you can flick it on or off as needed without disabling the plugin. (So you can avoid triggers on test-records etc).
