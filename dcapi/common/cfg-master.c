/*
 * cfg-client.c
 *
 * Simple config file parser for the client
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <dc_internal.h>
#include <dc.h>

#include <glib.h>

/********************************************************************
 * Constants
 */

/* Name of the group holding the master's configuration */
#define MASTER_GROUP		"Master"


/********************************************************************
 * Global variables
 */

static GKeyFile *config;


/********************************************************************
 * Implementation
 */

int _DC_parseCfg(const char *cfgfile)
{
	GError *error = NULL;
	int ret;

	/* Should not happen */
	if (!cfgfile)
		return DC_ERR_INTERNAL;

	config = g_key_file_new();
	ret = g_key_file_load_from_file(config, cfgfile, G_KEY_FILE_NONE,
		&error);
	if (!ret)
	{
		DC_log(LOG_ERR, "Failed to load the config file %s: %s",
			cfgfile, error->message);
		g_error_free(error);
		return DC_ERR_CONFIG;
	}
	return 0;
}

static char *getCfgStr(const char *group, const char *key)
{
	char *value, *tmp;

	if (!config || !key)
		return NULL;

	value = g_key_file_get_value(config, group, key, NULL);
	if (!value || g_mem_is_system_malloc())
		return value;
	tmp = strdup(value);
	g_free(value);
	return tmp;
}

static int getCfgInt(const char *group, const char *key, int defaultValue,
	int *err)
{
	char *value, *p;
	long retval;

	if (!config || !key)
		return defaultValue;

	value = g_key_file_get_value(config, group, key, NULL);
	if (!value)
	{
		*err = 1;
		return defaultValue;
	}

	retval = strtol(value, &p, 10);
	/* Check for unit suffixes */
	if (p && *p && _DC_processSuffix(&retval, p))
	{
		DC_log(LOG_WARNING, "Configuration value for key %s is not "
			"a valid number, ignoring", key);
		g_free(value);
		*err = 1;
		return defaultValue;
	}

	g_free(value);
	*err = 0;
	return retval;
}

char *DC_getCfgStr(const char *key)
{
	return getCfgStr(MASTER_GROUP, key);
}

int DC_getCfgInt(const char *key, int defaultValue)
{
	int err;

	return getCfgInt(MASTER_GROUP, key, defaultValue, &err);
}

char *DC_getClientCfgStr(const char *clientName, const char *key,
	int fallbackGlobal)
{
	char *group, *val;

	if (!clientName)
		return NULL;

	group = g_strdup_printf("Client-%s", clientName);
	val = getCfgStr(group, key);
	g_free(group);
	if (!val && fallbackGlobal)
		val = getCfgStr(MASTER_GROUP, key);
	return val;
}

int DC_getClientCfgInt(const char *clientName, const char *key,
	int defaultValue, int fallbackGlobal)
{
	int val, err;
	char *group;

	if (!clientName)
		return defaultValue;

	group = g_strdup_printf("Client-%s", clientName);
	val = getCfgInt(group, key, defaultValue, &err);
	g_free(group);
	if (err)
		val = getCfgInt(MASTER_GROUP, key, defaultValue, &err);
	return val;
}