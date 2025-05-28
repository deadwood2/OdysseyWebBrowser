#pragma once
#include "WebKit.h"

#if ENABLE(GAMEPAD)
#include <WebCore/GamepadProvider.h>
#include <WebCore/GamepadProviderClient.h>
#include <WebCore/PlatformGamepad.h>

/*
** Gamepad Lib - a sensors.library proxy for gamepads
** (C)2020 Jacek Piszczek / MorphOS Team
*/

#include <exec/types.h>

namespace WebKit {

#pragma pack(1)

typedef struct _gmlibStick
{
	DOUBLE _eastWest;
	DOUBLE _northSouth;
} gmlibStick;

typedef struct _gmlibButtons
{
	ULONG      _dummy : 18;

	ULONG      _shoulderRight : 1;
	ULONG      _shoulderLeft : 1;

	ULONG      _bRight : 1;
	ULONG      _aBottom : 1;
	ULONG      _yTop : 1;
	ULONG      _xLeft : 1;
		
	ULONG      _rightStickButton : 1;
	ULONG      _leftStickButton : 1;
	ULONG      _start : 1;
	ULONG      _back : 1;

	ULONG      _dpadDown : 1;
	ULONG      _dpadUp : 1;
	ULONG      _dpadRight : 1;
	ULONG      _dpadLeft : 1;
} gmlibButtons;

// This is essentially a mapping of a standard console gamepad
// HID devices will be approximated to this mapping as best as possible
typedef struct _gmlibGamepadData
{
	// Two analog joysticks
	gmlibStick _leftStick;
	gmlibStick _rightStick;

	// Two analog triggers
	DOUBLE     _leftTrigger;
	DOUBLE     _rightTrigger;

	// Buttons
	union {
		gmlibButtons _bits;
		ULONG _all;
	} _buttons;
	
	// Battery state
	DOUBLE     _battery;
} gmlibGamepadData;

typedef struct _gmlibGamepad
{
	char  _name[128];
	UWORD _pid;
	UWORD _vid;
	BOOL  _hasRumble;
	BOOL  _hasBattery;
} gmlibGamepad;

#pragma pack(0)

typedef struct _gmlibHandle
{
	void *_nope;
} gmlibHandle;

// Up to 4 gamepads can be handled at once
#define gmlibSlotMin (1)
#define gmlibSlotMax (4)

// Initialize gmlib, flags shall be 0 for now.
// The gameID should be a unique string identifying the game and may be used to provide
// per-game settings of input mappings etc
gmlibHandle *gmlibInitialize(const char *gameID, ULONG flags);

// MUST be called on the same thread that Initialize was called on
void gmlibShutdown(gmlibHandle *handle);

// Dedicated once-per-frame update function
// MUST be called on the same thread that Initialize was called on
// MUST be called ONCE before polling states of gamepads with gmlibGetData (there's no need
// to poll unused gamepads)
void gmlibUpdate(gmlibHandle *handle);

// Query for the gamepad at given slot; the gamepad data shall be written to provided storage
// If a gamepad gets removed, the slot won't be filled by shifting other gamepads, unless
// gmlibRenumerate is called. If a gamepad is plugged in, it will fill the 1st available slot.
BOOL gmlibGetGamepad(gmlibHandle *handle, ULONG slot, gmlibGamepad *outGamepad);

// Re-numerate gamepads, this will re-assign their slots to leave no gaps
// Do NOT call this when playing the game. The only moment where you want to call this is when
// displaying input settings
void gmlibRenumerate(gmlibHandle *handle);

// Obtains current gamepad inputs, normally to be called once per frame
void gmlibGetData(gmlibHandle *handle, ULONG slot, gmlibGamepadData *outData);

// Controls gamepad's motors, if present
void gmlibSetRumble(gmlibHandle *handle, ULONG slot, DOUBLE smallMotorPower, DOUBLE largeMotorPower, ULONG msDuration);

/*****************************************************************************************************/

class GamepadProviderMorphOS : public WebCore::GamepadProvider {
    WTF_MAKE_NONCOPYABLE(GamepadProviderMorphOS);
    friend class NeverDestroyed<GamepadProviderMorphOS>;
public:
    static GamepadProviderMorphOS& singleton();

    void startMonitoringGamepads(WebCore::GamepadProviderClient&) final;
    void stopMonitoringGamepads(WebCore::GamepadProviderClient&) final;
    const Vector<WebCore::PlatformGamepad*>& platformGamepads() final { return m_connectedGamepadVector; }
    
protected:
	GamepadProviderMorphOS();
	~GamepadProviderMorphOS();

    Vector<WebCore::PlatformGamepad*> m_connectedGamepadVector;
    gmlibHandle *m_handle = nullptr;
};

}
#endif

