/* gameplaySP
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "common.h"

// Special thanks to psp298 for the analog->dpad code!

void trigger_key(u32 key)
{
  u32 p1_cnt = io_registers[REG_P1CNT];

  if((p1_cnt >> 14) & 0x01)
  {
    u32 key_intersection = (p1_cnt & key) & 0x3FF;

    if(p1_cnt >> 15)
    {
      if(key_intersection == (p1_cnt & 0x3FF))
        raise_interrupt(IRQ_KEYPAD);
    }
    else
    {
      if(key_intersection)
        raise_interrupt(IRQ_KEYPAD);
    }
  }
}

u32 key = 0;

u32 global_enable_analog = 1;
u32 analog_sensitivity_level = 4;

typedef enum
{
  BUTTON_NOT_HELD,
  BUTTON_HELD_INITIAL,
  BUTTON_HELD_REPEAT
} button_repeat_state_type;


// These define autorepeat values (in microseconds), tweak as necessary.

#define BUTTON_REPEAT_START    200000
#define BUTTON_REPEAT_CONTINUE 50000

button_repeat_state_type button_repeat_state = BUTTON_NOT_HELD;
u32 button_repeat = 0;
gui_action_type cursor_repeat = CURSOR_NONE;

u32 gui_joy_map(u32 button)
{
  switch(button)
  {
    case SDL_CONTROLLER_BUTTON_A:
    case SDL_CONTROLLER_BUTTON_START:
      return CURSOR_SELECT;
    case SDL_CONTROLLER_BUTTON_BACK:
    case SDL_CONTROLLER_BUTTON_GUIDE:
    case SDL_CONTROLLER_BUTTON_B:
    case SDL_CONTROLLER_BUTTON_X:
    case SDL_CONTROLLER_BUTTON_Y:
      return CURSOR_EXIT;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
      return CURSOR_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
      return CURSOR_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
      return CURSOR_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
      return CURSOR_RIGHT;
    default:
      return CURSOR_NONE;
  }
}

gui_action_type get_gui_input()
{
  SDL_Event event;
  gui_action_type gui_action = CURSOR_NONE;

  delay_us(30000);

  while(SDL_PollEvent(&event))
  {
    switch(event.type) {
    case SDL_CONTROLLERBUTTONDOWN:
      gui_action = gui_joy_map(event.cbutton.button);
      break;
    case SDL_CONTROLLERAXISMOTION:
      if (event.caxis.axis==SDL_CONTROLLER_AXIS_LEFTX) { //Left-Right
        if (event.caxis.value < -3200)  gui_action = CURSOR_LEFT;
        else if (event.caxis.value > 3200)  gui_action = CURSOR_RIGHT;
      }
      if (event.caxis.axis==SDL_CONTROLLER_AXIS_LEFTY) {  //Up-Down
        if (event.caxis.value < -3200)  gui_action = CURSOR_UP;
        else if (event.caxis.value > 3200)  gui_action = CURSOR_DOWN;
      }
      break;
    case SDL_QUIT:
      quit();
      break;
    default:
      break;
    }
  }
  return gui_action;
}


u32 joy_map(u32 button)
{
  switch(button)
  {
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
      return BUTTON_L;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
      return BUTTON_R;
    case SDL_CONTROLLER_BUTTON_START:
      return BUTTON_START;
    case SDL_CONTROLLER_BUTTON_BACK:
    case SDL_CONTROLLER_BUTTON_GUIDE:
      return BUTTON_SELECT;
    case SDL_CONTROLLER_BUTTON_B:
      return BUTTON_B;
    case SDL_CONTROLLER_BUTTON_A:
      return BUTTON_A;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
      return BUTTON_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
      return BUTTON_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
      return BUTTON_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
      return BUTTON_RIGHT;
    default:
      return BUTTON_NONE;
  }
}

u32 update_input()
{
  SDL_Event event;

  while(SDL_PollEvent(&event))
  {
    switch(event.type) {
      case SDL_CONTROLLERAXISMOTION:
        if (event.caxis.axis==SDL_CONTROLLER_AXIS_LEFTX) { //Left-Right
          key &= ~(BUTTON_LEFT|BUTTON_RIGHT);
          if (event.caxis.value < -3200)  key |= BUTTON_LEFT;
          else if (event.caxis.value > 3200)  key |= BUTTON_RIGHT;
        }
        if (event.caxis.axis==SDL_CONTROLLER_AXIS_LEFTY) {  //Up-Down
          key &= ~(BUTTON_UP|BUTTON_DOWN);
          if (event.caxis.value < -3200)  key |= BUTTON_UP;
          else if (event.caxis.value > 3200)  key |= BUTTON_DOWN;
        }
        break;
      case SDL_CONTROLLERBUTTONDOWN:
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_Y) {
          u16 *screen_copy = copy_screen();
          u32 ret_val = menu(screen_copy);
          free(screen_copy);

          return ret_val;
        }
        key |= joy_map(event.cbutton.button);
        trigger_key(key);
        break;
      case SDL_CONTROLLERBUTTONUP:
        key &= ~(joy_map(event.cbutton.button));
        break;
      case SDL_QUIT:
        quit();
        break;
      default:
        break;
    }
  }

  io_registers[REG_P1] = (~key) & 0x3FF;

  return 0;
}

void init_input()
{
  char* joystick_active;
  u32 joystick_count = SDL_NumJoysticks();
  char guid_string[40];
  printf("Number of joysticks found: %d\r\n", joystick_count);
  printf("Total mappings: %d\r\n", SDL_GameControllerNumMappings());

  for (int i=0; i < joystick_count; i++) {
    SDL_GameController *gamecontroller = SDL_GameControllerOpen(i);

    SDL_JoystickGUID joystick_guid = SDL_JoystickGetDeviceGUID(i);
    memset(guid_string, 0, sizeof(guid_string[0]) * 40);
    SDL_JoystickGetGUIDString(joystick_guid, guid_string, 40);

    printf("Joystick Name: %s\n", SDL_JoystickNameForIndex(i));
    printf("  GUID: %s\n", guid_string);
    printf("  IsGameController: %d\n", SDL_IsGameController(i));

    SDL_JoystickGUID gamecontroller_guid = SDL_JoystickGetGUID((SDL_Joystick *)gamecontroller);
    const char * mapping = SDL_GameControllerMappingForGUID(joystick_guid);

    memset(guid_string, 0, sizeof(guid_string[0]) * 40);
    SDL_JoystickGetGUIDString(gamecontroller_guid, guid_string, 40);

    printf("Gamecontroller Name: %s\n", SDL_GameControllerName(gamecontroller));
    printf("  GUID: %s\n", guid_string);
    printf("  GameControllerPath: %s\n", SDL_GameControllerPath(gamecontroller));
    printf("  Serial: %s\n", SDL_GameControllerGetSerial(gamecontroller));
    printf("  Vid Pid: %d-%d\n", SDL_GameControllerGetVendor(gamecontroller), SDL_GameControllerGetProduct(gamecontroller));
    printf("  Mapping: %s\n", mapping);
    printf("\n");
  }
}

#define input_savestate_builder(type)                                         \
void input_##type##_savestate(file_tag_type savestate_file)                   \
{                                                                             \
  file_##type##_variable(savestate_file, key);                                \
}                                                                             \

input_savestate_builder(read);
input_savestate_builder(write_mem);
