using System;
using Kolrabi.SteamController;

namespace Test
{
	class MainClass
	{
		public static void Main (string[] args)
		{
			Console.WriteLine ("Hello World!");

			var controllers = SteamControllerDevice.OpenControllers ();

			while (true) {
				int i = 0;
				foreach (var controller in controllers) {
					var evt = controller.ReadEvent ();
					if (evt != null) {
						if (evt is ConnectionEvent && ((ConnectionEvent)evt).State == WirelessState.Connected) {
							controller.Configuration = 0;
						}
						controller.UpdateState (evt);
						Console.WriteLine ("Controller " + i);
						Console.WriteLine ("  Buttons:  " + controller.ButtonState);
						Console.WriteLine ("  Triggers: " + controller.LeftTrigger + ", " + controller.RightTrigger);
						Console.WriteLine ("  Stick:    " + controller.Stick.x + ", " + controller.Stick.y);
						Console.WriteLine ("  LPad:     " + controller.LeftPad.x + ", " + controller.LeftPad.y);
						Console.WriteLine ("  RPad:     " + controller.RightPad.x + ", " + controller.RightPad.y);
						Console.WriteLine ();
					}
					i++;
				}

			}
		}
	}
}
