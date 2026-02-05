#include "keyboard.h"
#include "ioport.h"
#include "interrupts.h"

static kb_key_state_t g_key_states[256];
static kb_event_t g_event_queue[100];
static k_usize_t g_event_queue_write_index;
static k_usize_t g_event_queue_read_index;

static void push_event(kb_event_t event) {
    g_event_queue[g_event_queue_write_index] = event;
    g_event_queue_write_index += 1;
    g_event_queue_write_index %= k_array_count(g_event_queue);

    if (g_event_queue_write_index == g_event_queue_read_index) {
        g_event_queue_read_index += 1;
        g_event_queue_read_index %= k_array_count(g_event_queue);
    }
}

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

    kb_event_t event = {};
    if (pressed) {
        if (state->repeat_count == 0) {
            event.type = KB_EVENT_PRESS;
        } else {
            event.type = KB_EVENT_REPEAT;
        }
    } else {
        event.type = KB_EVENT_RELEASE;
    }

    event.repeat_count = state->repeat_count;
    event.scancode = scancode;
    push_event(event);

    if (pressed) {
        char ascii_value = kb_translate_key_press(scancode, kb_get_mod_state());
        if (ascii_value) {
            kb_event_t text_input = {};
            text_input.type = KB_EVENT_TEXT_INPUT;
            text_input.ascii_value = ascii_value;
            push_event(text_input);
        }
    }
}

void kb_initialize() {
	isr_register_handler(IRQ_INDEX_KEYBOARD_CONTROLLER, handle_keyboard_irq);
	k_printf("Initialized keyboard controller\n");
}

kb_key_state_t kb_get_key_state(kb_scancode_t scancode) {
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

bool kb_poll_event(kb_event_t *event) {
    if (g_event_queue_read_index == g_event_queue_write_index) {
        return false;
    }

    *event = g_event_queue[g_event_queue_read_index];
    g_event_queue_read_index += 1;
    g_event_queue_read_index %= k_array_count(g_event_queue);

    return true;
}

char kb_translate_key_press(kb_scancode_t scancode, kb_mod_state_t mods) {
    switch (scancode) {
    case KB_SCANCODE_A: return mods & KB_MOD_SHIFT ? 'A' : 'a';
    case KB_SCANCODE_B: return mods & KB_MOD_SHIFT ? 'B' : 'b';
    case KB_SCANCODE_C: return mods & KB_MOD_SHIFT ? 'C' : 'c';
    case KB_SCANCODE_D: return mods & KB_MOD_SHIFT ? 'D' : 'd';
    case KB_SCANCODE_E: return mods & KB_MOD_SHIFT ? 'E' : 'e';
    case KB_SCANCODE_F: return mods & KB_MOD_SHIFT ? 'F' : 'f';
    case KB_SCANCODE_G: return mods & KB_MOD_SHIFT ? 'G' : 'g';
    case KB_SCANCODE_H: return mods & KB_MOD_SHIFT ? 'H' : 'h';
    case KB_SCANCODE_I: return mods & KB_MOD_SHIFT ? 'I' : 'i';
    case KB_SCANCODE_J: return mods & KB_MOD_SHIFT ? 'J' : 'j';
    case KB_SCANCODE_K: return mods & KB_MOD_SHIFT ? 'K' : 'k';
    case KB_SCANCODE_L: return mods & KB_MOD_SHIFT ? 'L' : 'l';
    case KB_SCANCODE_M: return mods & KB_MOD_SHIFT ? 'M' : 'm';
    case KB_SCANCODE_N: return mods & KB_MOD_SHIFT ? 'N' : 'n';
    case KB_SCANCODE_O: return mods & KB_MOD_SHIFT ? 'O' : 'o';
    case KB_SCANCODE_P: return mods & KB_MOD_SHIFT ? 'P' : 'p';
    case KB_SCANCODE_Q: return mods & KB_MOD_SHIFT ? 'Q' : 'q';
    case KB_SCANCODE_R: return mods & KB_MOD_SHIFT ? 'R' : 'r';
    case KB_SCANCODE_S: return mods & KB_MOD_SHIFT ? 'S' : 's';
    case KB_SCANCODE_T: return mods & KB_MOD_SHIFT ? 'T' : 't';
    case KB_SCANCODE_U: return mods & KB_MOD_SHIFT ? 'U' : 'u';
    case KB_SCANCODE_V: return mods & KB_MOD_SHIFT ? 'V' : 'v';
    case KB_SCANCODE_W: return mods & KB_MOD_SHIFT ? 'W' : 'w';
    case KB_SCANCODE_X: return mods & KB_MOD_SHIFT ? 'X' : 'x';
    case KB_SCANCODE_Y: return mods & KB_MOD_SHIFT ? 'Y' : 'y';
    case KB_SCANCODE_Z: return mods & KB_MOD_SHIFT ? 'Z' : 'z';
    case KB_SCANCODE_SPACE: return ' ';
    }

    return 0;
}
