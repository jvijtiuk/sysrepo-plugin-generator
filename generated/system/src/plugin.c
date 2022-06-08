#include "plugin.h"
#include "plugin/common.h"
#include "plugin/context.h"

// startup
#include "plugin/startup/load.h"
#include "plugin/startup/store.h"

// subscription
#include "plugin/subscription/change.h"
#include "plugin/subscription/operational.h"
#include "plugin/subscription/rpc.h"

#include <libyang/libyang.h>
#include <sysrepo.h>
#include <srpc.h>

int sr_plugin_init_cb(sr_session_ctx_t *running_session, void **private_data)
{
	int error = 0;

	bool empty_startup = false;

	// sysrepo
	sr_session_ctx_t *startup_session = NULL;
	sr_conn_ctx_t *connection = NULL;
	sr_subscription_ctx_t *subscription = NULL;

	// plugin
	system_ctx_t *ctx = NULL;

	// init context
	ctx = malloc(sizeof(*ctx));
	*ctx = (system_ctx_t){0};

	*private_data = ctx;

	// module changes
	srpc_module_change_t module_changes[] = {
        {
            SYSTEM_SYSTEM_CONTACT_YANG_PATH,
            system_subscription_change_system_contact,
        },
        {
            SYSTEM_SYSTEM_HOSTNAME_YANG_PATH,
            system_subscription_change_system_hostname,
        },
        {
            SYSTEM_SYSTEM_LOCATION_YANG_PATH,
            system_subscription_change_system_location,
        },
        {
            SYSTEM_SYSTEM_CLOCK_TIMEZONE_NAME_YANG_PATH,
            system_subscription_change_system_clock_timezone_name,
        },
        {
            SYSTEM_SYSTEM_CLOCK_TIMEZONE_UTC_OFFSET_YANG_PATH,
            system_subscription_change_system_clock_timezone_utc_offset,
        },
        {
            SYSTEM_SYSTEM_NTP_ENABLED_YANG_PATH,
            system_subscription_change_system_ntp_enabled,
        },
        {
            SYSTEM_SYSTEM_NTP_SERVER_YANG_PATH,
            system_subscription_change_system_ntp_server,
        },
        {
            SYSTEM_SYSTEM_DNS_RESOLVER_SEARCH_YANG_PATH,
            system_subscription_change_system_dns_resolver_search,
        },
        {
            SYSTEM_SYSTEM_DNS_RESOLVER_SERVER_YANG_PATH,
            system_subscription_change_system_dns_resolver_server,
        },
        {
            SYSTEM_SYSTEM_DNS_RESOLVER_OPTIONS_TIMEOUT_YANG_PATH,
            system_subscription_change_system_dns_resolver_options_timeout,
        },
        {
            SYSTEM_SYSTEM_DNS_RESOLVER_OPTIONS_ATTEMPTS_YANG_PATH,
            system_subscription_change_system_dns_resolver_options_attempts,
        },
        {
            SYSTEM_SYSTEM_RADIUS_SERVER_YANG_PATH,
            system_subscription_change_system_radius_server,
        },
        {
            SYSTEM_SYSTEM_RADIUS_OPTIONS_TIMEOUT_YANG_PATH,
            system_subscription_change_system_radius_options_timeout,
        },
        {
            SYSTEM_SYSTEM_RADIUS_OPTIONS_ATTEMPTS_YANG_PATH,
            system_subscription_change_system_radius_options_attempts,
        },
        {
            SYSTEM_SYSTEM_AUTHENTICATION_USER_AUTHENTICATION_ORDER_YANG_PATH,
            system_subscription_change_system_authentication_user_authentication_order,
        },
        {
            SYSTEM_SYSTEM_AUTHENTICATION_USER_YANG_PATH,
            system_subscription_change_system_authentication_user,
        },
	};

	// rpcs
	srpc_rpc_t rpcs[] = {
        {
            SYSTEM_SET_CURRENT_DATETIME_YANG_PATH,
            system_subscription_rpc_set_current_datetime,
        },
        {
            SYSTEM_SYSTEM_RESTART_YANG_PATH,
            system_subscription_rpc_system_restart,
        },
        {
            SYSTEM_SYSTEM_SHUTDOWN_YANG_PATH,
            system_subscription_rpc_system_shutdown,
        },
	};

	// operational getters
	srpc_operational_t oper[] = {
        {
            SYSTEM_SYSTEM_STATE_PLATFORM_OS_NAME_YANG_PATH,
            system_subscription_operational_system_state_platform_os_name,
        },
        {
            SYSTEM_SYSTEM_STATE_PLATFORM_OS_RELEASE_YANG_PATH,
            system_subscription_operational_system_state_platform_os_release,
        },
        {
            SYSTEM_SYSTEM_STATE_PLATFORM_OS_VERSION_YANG_PATH,
            system_subscription_operational_system_state_platform_os_version,
        },
        {
            SYSTEM_SYSTEM_STATE_PLATFORM_MACHINE_YANG_PATH,
            system_subscription_operational_system_state_platform_machine,
        },
        {
            SYSTEM_SYSTEM_STATE_CLOCK_CURRENT_DATETIME_YANG_PATH,
            system_subscription_operational_system_state_clock_current_datetime,
        },
        {
            SYSTEM_SYSTEM_STATE_CLOCK_BOOT_DATETIME_YANG_PATH,
            system_subscription_operational_system_state_clock_boot_datetime,
        },
	};

	connection = sr_session_get_connection(running_session);
	error = sr_session_start(connection, SR_DS_STARTUP, &startup_session);
	if (error) {
		SRPLG_LOG_ERR(PLUGIN_NAME, "sr_session_start() error (%d): %s", error, sr_strerror(error));
		goto error_out;
	}

	ctx->startup_session = startup_session;

	error = srpc_check_empty_datastore(startup_session, "[ENTER_PATH_TO_CHECK]", &empty_startup);
	if (error) {
		SRPLG_LOG_ERR(PLUGIN_NAME, "Failed checking datastore contents: %d", error);
		goto error_out;
	}

	if (empty_startup) {
		SRPLG_LOG_INF(PLUGIN_NAME, "Startup datasore is empty");
		SRPLG_LOG_INF(PLUGIN_NAME, "Loading initial system data");
		error = system_startup_load(ctx, startup_session);
		if (error) {
			SRPLG_LOG_ERR(PLUGIN_NAME, "Error loading initial data into the startup datastore... exiting");
			goto error_out;
		}

		// copy contents of the startup session to the current running session
		error = sr_copy_config(running_session, BASE_YANG_MODEL, SR_DS_STARTUP, 0);
		if (error) {
			SRPLG_LOG_ERR(PLUGIN_NAME, "sr_copy_config() error (%d): %s", error, sr_strerror(error));
			goto error_out;
		}
	} else {
		// make sure the data from startup DS is stored in the system
		SRPLG_LOG_INF(PLUGIN_NAME, "Startup datasore contains data");
		SRPLG_LOG_INF(PLUGIN_NAME, "Storing startup datastore data in the system");

		error = system_startup_store(ctx, startup_session);
		if (error) {
			SRPLG_LOG_ERR(PLUGIN_NAME, "Error applying initial data from startup datastore to the system... exiting");
			goto error_out;
		}

		// copy contents of the startup session to the current running session
		error = sr_copy_config(running_session, BASE_YANG_MODEL, SR_DS_STARTUP, 0);
		if (error) {
			SRPLG_LOG_ERR(PLUGIN_NAME, "sr_copy_config() error (%d): %s", error, sr_strerror(error));
			goto error_out;
		}
	}

	// subscribe every module change
	for (size_t i = 0; i < ARRAY_SIZE(module_changes); i++) {
		const srpc_module_change_t *change = &module_changes[i];

		// in case of work on a specific callback set it to NULL
		if (change->cb) {
			error = sr_module_change_subscribe(running_session, BASE_YANG_MODEL, change->path, change->cb, *private_data, 0, SR_SUBSCR_DEFAULT, &subscription);
			if (error) {
				SRPLG_LOG_ERR(PLUGIN_NAME, "sr_module_change_subscribe() error for \"%s\" (%d): %s", change->path, error, sr_strerror(error));
				goto error_out;
			}
		}
	}

	// subscribe every rpc
	for (size_t i = 0; i < ARRAY_SIZE(rpcs); i++) {
		const srpc_rpc_t *rpc = &rpcs[i];

		// in case of work on a specific callback set it to NULL
		if (rpc->cb) {
			error = sr_rpc_subscribe(running_session, rpc->path, rpc->cb, *private_data, 0, SR_SUBSCR_DEFAULT, &subscription);
			if (error) {
				SRPLG_LOG_ERR(PLUGIN_NAME, "sr_rpc_subscribe error (%d): %s", error, sr_strerror(error));
				goto error_out;
			}
		}
	}

	// subscribe every operational getter
	for (size_t i = 0; i < ARRAY_SIZE(oper); i++) {
		const srpc_operational_t *op = &oper[i];

		// in case of work on a specific callback set it to NULL
		if (op->cb) {
			error = sr_oper_get_subscribe(running_session, BASE_YANG_MODEL, op->path, op->cb, NULL, SR_SUBSCR_DEFAULT, &subscription);
			if (error) {
				SRPLG_LOG_ERR(PLUGIN_NAME, "sr_oper_get_subscribe() error (%d): %s", error, sr_strerror(error));
				goto error_out;
			}
		}
	}

	goto out;

error_out:
	error = -1;
	SRPLG_LOG_ERR(PLUGIN_NAME, "Error occured while initializing the plugin (%d)", error);

out:
	return error ? SR_ERR_CALLBACK_FAILED : SR_ERR_OK;
}

void sr_plugin_cleanup_cb(sr_session_ctx_t *running_session, void *private_data)
{
	int error = 0;

	system_ctx_t *ctx = (system_ctx_t *) private_data;

	// save current running configuration into startup for next time when the plugin starts
	error = sr_copy_config(ctx->startup_session, BASE_YANG_MODEL, SR_DS_RUNNING, 0);
	if (error) {
		SRPLG_LOG_ERR(PLUGIN_NAME, "sr_copy_config() error (%d): %s", error, sr_strerror(error));
	}

	free(ctx);
}