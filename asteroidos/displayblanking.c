#include <stdbool.h>
#include <stdio.h>

#include <dbus/dbus.h>

#include <mce/dbus-names.h>
#include <mce/mode-names.h>

static DBusConnection *connection = NULL;

static DBusConnection *displayblanking_init(void)
{
  if (connection) return connection;

  DBusError err = DBUS_ERROR_INIT;
  DBusBusType bus_type = DBUS_BUS_SYSTEM;

  connection = dbus_bus_get(bus_type, &err);
  if (!connection) {
    printf("Failed to open connection to message bus; %s: %s\n", err.name, err.message);
    dbus_error_free(&err);
    return NULL;
  }

  if (!dbus_bus_name_has_owner(connection, MCE_SERVICE, &err)) {
    if (dbus_error_is_set(&err)) {
      printf("%s: %s: %s\n", MCE_SERVICE, err.name, err.message);
    }
    printf("MCE not running.\n");
    dbus_error_free(&err);
    return NULL;
  }

  return connection;
}

void displayblanking_exit(void)
{
  if (connection != NULL) {
    dbus_connection_unref(connection);
    connection = NULL;
  }
}

void displayblanking_prevent(void)
{
  const char *service = MCE_SERVICE;
  const char *path = MCE_REQUEST_PATH;
  const char *interface = MCE_REQUEST_IF;
  const char *name = MCE_PREVENT_BLANK_REQ;
  DBusMessage *msg = 0;
  DBusConnection *bus = displayblanking_init();
  DBusError err = DBUS_ERROR_INIT;

  msg = dbus_message_new_method_call(service, path, interface, name);

  dbus_message_set_auto_start(msg, FALSE);

  dbus_message_set_no_reply(msg, TRUE);

  if (!dbus_connection_send(bus, msg, NULL)) {
    printf("Failed to send method call\n");
    goto EXIT;
  }
  dbus_connection_flush(bus);

EXIT:
  dbus_error_free(&err);

  if (msg) dbus_message_unref(msg);
}
