#include "keyboard.h"
#include "ioport.h"
#include "interrupts.h"

static kb_key_state_t g_key_states[256];

static void handle_keyboard_irq(isr_registers_t registers) {
	(void)registers;

	uint8_t scancode = ioport_read_byte(0x60);
	bool pressed = (scancode & 0x80) == 0;
	scancode &= ~0x80;

	kb_key_state_t *state = &g_key_states[scancode];
	if (state->pressed && pressed) {
		state->repeat_count += 1;

		if (state->repeat_count == 0) { // In case of overflow
			state->repeat_count = 1;
		}
	} else {
		state->pressed = pressed;
		state->repeat_count = 0;
	}
}

void kb_initialize() {
	isr_register_handler(IRQ_INDEX_KEYBOARD_CONTROLLER, handle_keyboard_irq);
	k_printf("Initialized keyboard controller\n");
}

kb_key_state_t kb_get_key_state(kb_scancode_t scancode) {
	if (scancode < 0 || scancode >= 256) {
		return (kb_key_state_t){};
	}

	return g_key_states[scancode];
}

bool kb_is_key_pressed(kb_scancode_t scancode) {
	return kb_get_key_state(scancode).pressed;
}

kb_mod_state_t kb_get_mod_state() {
	kb_mod_state_t mods = 0;
	if (g_key_states[KB_SCANCODE_LSHIFT].pressed || g_key_states[KB_SCANCODE_RSHIFT].pressed) {
		mods |= KB_MOD_SHIFT;
	}
	if (g_key_states[KB_SCANCODE_ALT].pressed) {
		mods |= KB_MOD_ALT;
	}
	if (g_key_states[KB_SCANCODE_CTRL].pressed) {
		mods |= KB_MOD_CTRL;
	}

	return mods;
}

static const char *g_key_names[] = {
    "Invalid",
    "Escape",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "Minus",
    "Equal",
    "Backspace",
    "Tab",
    "Q",
    "W",
    "E",
    "R",
    "T",
    "Y",
    "U",
    "I",
    "O",
    "P",
    "OpenBracket",
    "CloseBracket",
    "Return",
    "Ctrl",
    "A",
    "S",
    "D",
    "F",
    "G",
    "H",
    "J",
    "K",
    "L",
    "Semicolon",
    "Quotes",
    "Backtick",
    "LeftShift",
    "Backslash",
    "Z",
    "X",
    "C",
    "V",
    "B",
    "N",
    "M",
    "Comma",
    "Period",
    "Slash",
    "RightShift",
    "PrintScreen",
    "Alt",
    "Space",
    "CapsLock",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "NumLock",
    "ScrollLock",
    "Home",
    "Up",
    "PageUp",
    "",
    "Left",
    "",
    "Right",
    "",
    "End",
    "Down",
    "PageDown",
    "Insert",
    "Delete",
};

const char *kb_get_key_name(kb_scancode_t scancode) {
	if (scancode >= k_array_count(g_key_names)) {
		return g_key_names[0];
	}

	return g_key_names[scancode];
}

static const char *g_mod_state_strings[] = {
    "",
    "Ctrl",
    "Alt",
    "Ctrl+Alt",
    "Shift",
    "Ctrl+Shift",
    "Alt+Shift",
    "Ctrl+Alt+Shift",
};

const char *kb_get_mod_state_string(kb_mod_state_t mods) {
	if (mods >= k_array_count(g_mod_state_strings)) {
		return g_mod_state_strings[0];
	}

	return g_mod_state_strings[mods];
}


