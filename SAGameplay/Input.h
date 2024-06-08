#pragma once
namespace GlobalInput
{
	class Input
	{
	public:
		static const uint16_t* heldButtons;
		static const uint16_t* pressedButtons;
		static const uint16_t* releasedButtons;

		static float* const leftStickX;
		static float* const leftStickY;

		static const float* rightStickX;
		static const float* rightStickY;

		static const float* leftTrigger;
		static const float* rightTrigger;

		enum Buttons
		{
			Jump = 0x01,
			Crouch = 0x02,
			Stomp = 0x02,
			Boost = 0x08,
			Lightdash = 0x10,

			A = 0x01,
			B = 0x02,
			X = 0x08,
			Y = 0x10,

			None = 0x0,

			DpadUp = 0x40,
			DpadDown = 0x80,
			DpadLeft = 0x100,
			DpadRight = 0x200,

			Start = 0x400,
			Select = 0x800,

			LeftBumper = 0x1000,
			RightBumper = 0x2000,

			LeftTrigger = 0x4000,
			RightTrigger = 0x8000,

			LeftStick = 0x10000,
			RightStick = 0x20000
		};
	};
}