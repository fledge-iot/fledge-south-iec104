#include <gtest/gtest.h>

#include <plugin_api.h>
#include <config_category.h>

#include <utility>
#include <vector>
#include <string>

#include "cs104_slave.h"

#include "iec104.h"
#include "iec104_client_config.h"
#include "iec104_utility.h"

using namespace std;

#define TEST_PORT 2404

// PLUGIN DEFAULT PROTOCOL STACK CONF
static string protocol_config = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : false,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken1 = QUOTE({
        "protocoll_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_nam" : "red-group1",
                        "tlsss" : false,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "cmd_exec_timeout": 0,
                "gi_repeat_count": 1,
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken2 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transpport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : false,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken3 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : false,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "appplication_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken4 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : false,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "cmd_exec_timeout": 0,
                "gi_repeat_count": 1,
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : true,
                "utc_time" : false,
                "cmd_with_timetag" : false,
                "cmd_parallel" : 0,
                "time_sync" : 1
            },
            "south_monitoring" : {
                "asset": "CONSTAT-1",
                "cnx_loss_status_id" : ""
            }
        }
    });

static string protocol_config_broken5 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.0.2",
                                "clt_ip" : "127.0.0",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : false,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 0,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken6 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 65637
                            },
                            {
                                "srv_ip" : "127.0.0.2",
                                "clt_ip" : "127.0.0.3",
                                "port" : 65638
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : false,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 0,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken7 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : "65637"
                            },
                            {
                                "srv_ip" : "127.0.0.2",
                                "clt_ip" : "127.0.0.3",
                                "port" : "65638"
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : false,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 0,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken8 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : false,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken9 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : false,
                        "k_value" : 32769,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken10 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : false,
                        "k_value" : "32769",
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken11 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : false,
                        "k_value" : 12,
                        "w_value" : 32769,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken12 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : false,
                        "k_value" : 12,
                        "w_value" : "32769",
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken13 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : false,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 257,
                        "t1_timeout" : 257,
                        "t2_timeout" : 257,
                        "t3_timeout" : -2
                    }
                ]
            },
            "application_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken14 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : false,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : "257",
                        "t1_timeout" : "257",
                        "t2_timeout" : "257",
                        "t3_timeout" : "257"
                    }
                ]
            },
            "application_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken15 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : 3,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
                "time_sync" : 0
            }
        }
    });


static string protocol_config_broken16 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : 3,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }

            },
            "application_layer" : {
                "orig_addr" : 10,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken17 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : 3,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "orig_addr" : 256,
                "ca_asdu_size" : 4,
                "ioaddr_size" : 5,
                "startup_time" : 180,
                "asdu_size" : 9,
                "gi_time" : -10,
                "gi_cycle" : -10,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
                "time_sync" : -10
            }
        }
    });

static string protocol_config_broken18 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : 3,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "cmd_exec_timeout": -10,
                "gi_repeat_count": -10,
                "orig_addr" : 256,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : -10,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_broken19 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            },
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : 3,
                        "k_value" : 12,
                        "w_value" : 8,
                        "t0_timeout" : 10,
                        "t1_timeout" : 15,
                        "t2_timeout" : 10,
                        "t3_timeout" : 20
                    }
                ]
            },
            "application_layer" : {
                "cmd_exec_timeout": "-10",
                "gi_repeat_count": "-10",
                "orig_addr" : "256",
                "ca_asdu_size" : "2",
                "ioaddr_size" : "3",
                "startup_time" : "180",
                "asdu_size" : "0",
                "gi_time" : "60",
                "gi_cycle" : "30",
                "gi_all_ca" : "false",
                "gi_enabled" : "false",
                "utc_time" : "false",
                "cmd_wttag" : "false",
                "cmd_parallel" : "-10",
                "time_sync" : "0"
            }
        }
    });

// PLUGIN DEFAULT EXCHANGED DATA CONF

static string exchanged_data = QUOTE({
        "exchanged_data": {
            "name" : "iec104client",
            "version" : "1.0",
            "datapoints" : [
                {
                    "label":"TM-1",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4202832",
                          "typeid":"M_ME_NA_1"
                       }
                    ]
                },
                {
                    "label":"TM-2",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4202852",
                          "typeid":"M_ME_NA_1"
                       }
                    ]
                },
                {
                    "label":"TS-1",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4206948",
                          "typeid":"M_SP_TB_1"
                       }
                    ]
                },
                {
                    "label":"C-1",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2000",
                          "typeid":"C_SC_NA_1"
                       }
                    ]
                },

                {
                    "label":"C-2",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2001",
                          "typeid":"C_SC_TA_1"
                       }
                    ]
                },
                {
                    "label":"C-3",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2002",
                          "typeid":"C_DC_NA_1"
                       }
                    ]
                },
                {
                    "label":"C-3",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2003",
                          "typeid":"C_SE_TC_1"
                       }
                    ]
                },
                {
                    "label":"C-5",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2004",
                          "typeid":"C_DC_TA_1"
                       }
                    ]
                },
                {
                    "label":"C-6",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2005",
                          "typeid":"C_RC_NA_1"
                       }
                    ]
                },
                {
                    "label":"C-13",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2012",
                          "typeid":"C_SE_NC_1"
                       }
                    ]
                },
                {
                    "label":"C-7",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2006",
                          "typeid":"C_SE_NA_1"
                       }
                    ]
                },
                {
                    "label":"C-8",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2007",
                          "typeid":"C_SE_TA_1"
                       }
                    ]
                },
                {
                    "label":"C-9",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2008",
                          "typeid":"C_SE_NB_1"
                       }
                    ]
                },
                {
                    "label":"C-10",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2009",
                          "typeid":"C_SE_TB_1"
                       }
                    ]
                },
                {
                    "label":"C-11",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2010",
                          "typeid":"C_SC_TA_1"
                       }
                    ]
                },
                {
                    "label":"C-12",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2011",
                          "typeid":"C_RC_TA_1"
                       },
                       {
                          "name":"iec104",
                          "address":"41025-2012",
                          "typeid":"C_RC_TA_1"
                       }
                    ]
                }
            ]
        }
    });

static string exchanged_data_2 = QUOTE({
        "exchanged_data": {
            "name" : "iec104client",
            "version" : "1.0",
            "datapoints" : [
                {
                    "label":"TM-1",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4202832",
                          "typeid":"M_ME_NA_1"
                       }
                    ]
                },
                {
                    "label":"TM-2",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4202852",
                          "typeid":"M_ME_NA_1"
                       }
                    ]
                },
                {
                    "label":"TS-1",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4206948",
                          "typeid":"M_SP_TB_1"
                       }
                    ]
                },
                {
                    "label":"C-1",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2000",
                          "typeid":"C_SC_NA_1"
                       }
                    ]
                },

                {
                    "label":"C-2",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2001",
                          "typeid":"C_SC_TA_1"
                       }
                    ]
                },
                {
                    "label":"C-3",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2002",
                          "typeid":"C_DC_NA_1"
                       }
                    ]
                },
                {
                    "label":"C-3",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2003",
                          "typeid":"C_SE_TC_1"
                       }
                    ]
                },
                {
                    "label":"C-5",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2004",
                          "typeid":"C_DC_TA_1"
                       }
                    ]
                },
                {
                    "label":"C-6",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2005",
                          "typeid":"C_RC_NA_1"
                       }
                    ]
                },
                {
                    "label":"C-13",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2012",
                          "typeid":"C_SE_NC_1"
                       }
                    ]
                },
                {
                    "label":"C-7",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2006",
                          "typeid":"C_SE_NA_1"
                       }
                    ]
                },
                {
                    "label":"C-8",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2007",
                          "typeid":"C_SE_TA_1"
                       }
                    ]
                },
                {
                    "label":"C-9",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2008",
                          "typeid":"C_SE_NB_1"
                       }
                    ]
                },
                {
                    "label":"C-10",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2009",
                          "typeid":"C_SE_TB_1"
                       }
                    ]
                },
                {
                    "label":"C-11",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2010",
                          "typeid":"C_SC_TA_1"
                       }
                    ]
                }
            ]
        }
    });

static string exchanged_data_broken1 = QUOTE({
            "name" : "iec104client",
            "version" : "1.0",
            "datapoints" : [
                {
                    "label":"TM-1",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4202832",
                          "typeid":"M_ME_NA_1"
                       }
                    ]
                },
                {
                    "label":"TM-2",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4202852",
                          "typeid":"M_ME_NA_1"
                       }
                    ]
                },
                {
                    "label":"TS-1",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4206948",
                          "typeid":"M_SP_TB_1"
                       }
                    ]
                },
                {
                    "label":"C-1",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2000",
                          "typeid":"C_SC_NA_1"
                       }
                    ]
                },

                {
                    "label":"C-2",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2001",
                          "typeid":"C_SC_TA_1"
                       }
                    ]
                },
                {
                    "label":"C-3",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2002",
                          "typeid":"C_DC_NA_1"
                       }
                    ]
                },
                {
                    "label":"C-3",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2003",
                          "typeid":"C_SE_TC_1"
                       }
                    ]
                },
                {
                    "label":"C-5",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2004",
                          "typeid":"C_DC_TA_1"
                       }
                    ]
                },
                {
                    "label":"C-6",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2005",
                          "typeid":"C_RC_NA_1"
                       }
                    ]
                },
                {
                    "label":"C-13",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2012",
                          "typeid":"C_SE_NC_1"
                       }
                    ]
                },
                {
                    "label":"C-7",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2006",
                          "typeid":"C_SE_NA_1"
                       }
                    ]
                },
                {
                    "label":"C-8",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2007",
                          "typeid":"C_SE_TA_1"
                       }
                    ]
                },
                {
                    "label":"C-9",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2008",
                          "typeid":"C_SE_NB_1"
                       }
                    ]
                },
                {
                    "label":"C-10",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2009",
                          "typeid":"C_SE_TB_1"
                       }
                    ]
                },
                {
                    "label":"C-11",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2010",
                          "typeid":"C_SC_TA_1"
                       }
                    ]
                },
                {
                    "label":"C-12",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2011",
                          "typeid":"C_RC_TA_1"
                       },
                       {
                          "name":"iec104",
                          "address":"41025-2012",
                          "typeid":"C_RC_TA_1"
                       }
                    ]
                }
            ]
    });

// PLUGIN DEFAULT TLS CONF
static string tls_config = QUOTE({
        "tls_conf" : {
            "private_key" : "iec104_client.key",
            "own_cert" : "iec104_client.cer",
            "ca_certs" : [
                {
                    "cert_file": "iec104_ca.cer"
                }
            ],
            "remote_certs" : [
                {
                    "cert_file": "iec104_server.cer"
                }
            ]
        }
    });

static string tls_config_broken1 = QUOTE({
        "private_key" : "iec104_client.key",
        "own_cert" : "iec104_client.cer",
        "ca_certs" : [
            {
                "cert_file": "iec104_ca.cer"
            }
        ],
        "remote_certs" : [
            {
                "cert_file": "iec104_server.cer"
            }
        ]
    });

class IEC104TestComp : public IEC104
{
public:
    IEC104TestComp() : IEC104()
    {
    }
};

class ConfigTest : public testing::Test
{
protected:

    void SetUp()
    {
        iec104 = new IEC104TestComp();
        iec104->setJsonConfig(protocol_config, exchanged_data, tls_config);

        iec104->registerIngest(NULL, ingestCallback);
    }

    void TearDown()
    {
        iec104->stop();

        delete iec104;

        for (Reading* reading : storedReadings)
        {
            delete reading;
        }

        storedReadings.clear();
    }

    void startIEC104() { iec104->start(); }

    static bool hasChild(Datapoint& dp, std::string childLabel)
    {
        DatapointValue& dpv = dp.getData();

        auto dps = dpv.getDpVec();

        for (auto sdp : *dps) {
            if (sdp->getName() == childLabel) {
                return true;
            }
        }

        return false;
    }

    static Datapoint* getChild(Datapoint& dp, std::string childLabel)
    {
        DatapointValue& dpv = dp.getData();

        auto dps = dpv.getDpVec();

        for (Datapoint* childDp : *dps) {
            if (childDp->getName() == childLabel) {
                return childDp;
            }
        }

        return nullptr;
    }

    static int64_t getIntValue(Datapoint* dp)
    {
        DatapointValue dpValue = dp->getData();
        return dpValue.toInt();
    }

    static std::string getStrValue(Datapoint* dp)
    {
        return dp->getData().toStringValue();
    }

    static bool hasObject(Reading& reading, std::string label)
    {
        std::vector<Datapoint*> dataPoints = reading.getReadingData();

        for (Datapoint* dp : dataPoints)
        {
            if (dp->getName() == label) {
                return true;
            }
        }

        return false;
    }

    static Datapoint* getObject(Reading& reading, std::string label)
    {
        std::vector<Datapoint*> dataPoints = reading.getReadingData();

        for (Datapoint* dp : dataPoints)
        {
            if (dp->getName() == label) {
                return dp;
            }
        }

        return nullptr;
    }

    static void ingestCallback(void* parameter, Reading reading)
    {
        printf("ingestCallback called -> asset: (%s)\n", reading.getAssetName().c_str());

        std::vector<Datapoint*> dataPoints = reading.getReadingData();

        // for (Datapoint* sdp : dataPoints) {
        //     printf("name: %s value: %s\n", sdp->getName().c_str(), sdp->getData().toString().c_str());
        // }
        storedReadings.push_back(new Reading(reading));

        ingestCallbackCalled++;
    }

    static bool clockSynchronizationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime)
    {
        clockSyncHandlerCalled++;

        return true;
    }

    static bool asduHandler (void* parameter, IMasterConnection connection, CS101_ASDU asdu)
    {
        ConfigTest* self = (ConfigTest*)parameter;

        printf("asduHandler: type: %i COT: %s\n", CS101_ASDU_getTypeID(asdu), CS101_CauseOfTransmission_toString(CS101_ASDU_getCOT(asdu)));

        lastConnection = NULL;
        lastOA = CS101_ASDU_getOA(asdu);

        int ca = CS101_ASDU_getCA(asdu);

        InformationObject io = CS101_ASDU_getElement(asdu, 0);

        int ioa = InformationObject_getObjectAddress(io);

        if (CS101_ASDU_getTypeID(asdu) == C_SC_NA_1) {
            printf("  C_SC_NA_1 (single-command)\n");

            if (ca == 41025 && ioa == 2000) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_SC_TA_1) {
            printf("  C_SC_TA_1 (single-command w/timetag)\n");

            if (ca == 41025 && ioa == 2010) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_DC_NA_1) {
            printf("  C_DC_NA_1 (double-command)\n");

            if (ca == 41025 && ioa == 2002) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_SE_TC_1) {
            printf("  C_SE_TC_1 (setpoint command short)\n");

            if (ca == 41025 && ioa == 2003) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
         else if (CS101_ASDU_getTypeID(asdu) == C_SE_NC_1) {
            printf("  C_SE_NC_1 (setpoint command short)\n");

            if (ca == 41025 && ioa == 2012) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_SE_NA_1) {
            printf("  C_SE_NA_1 (setpoint command normalized)\n");

            if (ca == 41025 && ioa == 2006) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_SE_TA_1) {
            printf("  C_SE_TA_1 (setpoint command normalized)\n");

            if (ca == 41025 && ioa == 2007) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_SE_NB_1) {
            printf("  C_SE_NB_1 (setpoint command scaled)\n");

            if (ca == 41025 && ioa == 2008) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_SE_TB_1) {
            printf("  C_SE_TB_1 (setpoint command scaled)\n");

            if (ca == 41025 && ioa == 2009) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_RC_TA_1) {
            printf("  C_RC_TA_1 (step command)\n");

            if (ca == 41025 && ioa == 2011) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_DC_TA_1) {
            printf("  C_DC_TA_1 (double-command)\n");

            if (ca == 41025 && ioa == 2004) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_RC_NA_1) {
            printf("  C_RC_NA_1 (step-command wo time)\n");

            if (ca == 41025 && ioa == 2005) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }

        InformationObject_destroy(io);

        if (CS101_ASDU_getTypeID(asdu) != C_IC_NA_1)
            asduHandlerCalled++;

        return true;
    }

    IEC104TestComp* iec104 = nullptr;
    static int ingestCallbackCalled;
    static std::vector<Reading*> storedReadings;
    static int clockSyncHandlerCalled;
    static int asduHandlerCalled;
    static IMasterConnection lastConnection;
    static int lastOA;

    static bool operation(const std::string& operation, int count, PLUGIN_PARAMETER** params)
    {
        printf("Entra aqui");
        IEC104ClientConfig* config = new IEC104ClientConfig();
        std::string type = params[0]->value;

        int typeID = config->GetTypeIdByName(type);

        return m_commandOperation(count, params, typeID, config);
    }

    static bool m_commandOperation(int count, PLUGIN_PARAMETER** params, int typeId, IEC104ClientConfig* config)
    {
        if (count > 8) {
            // common address of the asdu
            int ca = atoi(params[1]->value.c_str());

            // information object address
            int32_t ioa = atoi(params[2]->value.c_str());

            // command state to send, must be a boolean
            // 0 = off, 1 otherwise
            bool value = static_cast<bool>(atoi(params[8]->value.c_str()));

            // select or execute, must be a boolean
            // 0 = execute, otherwise = select
            bool select = static_cast<bool>(atoi(params[5]->value.c_str()));

            long time = 0;

            if(params[7] != 0)
                time = stol(params[7]->value);

            // Iec104Utility::log_debug("operate: single command - CA: %i IOA: %i value: %i select: %i timestamp: %i", ca, ioa, value, select, time);

            return checkTypeCommand(ca, ioa, value, select, time, typeId, config);
        }
        else {
            Iec104Utility::log_error("operation parameter missing");
            return false;
        }
    }

    static bool checkTypeCommand(int ca, int ioa, bool value, bool select, long time, int typeId, IEC104ClientConfig* config)
    {
        // check if the data point is in the exchange configuration
        if (config->checkExchangeDataLayer(typeId, ca, ioa) == nullptr) {
            Iec104Utility::log_error("Failed to send C_SC_NA_1 command - no such data point");

            return false;
        }
        printf("MOSSSSSSSSSSSSS\n");

        return true;
    }
};

int ConfigTest::ingestCallbackCalled;
std::vector<Reading*> ConfigTest::storedReadings;
int ConfigTest::asduHandlerCalled;
int ConfigTest::clockSyncHandlerCalled;
IMasterConnection ConfigTest::lastConnection;
int ConfigTest::lastOA;

TEST_F(ConfigTest, ConfigTest1) {
    iec104->setJsonConfig(protocol_config_broken1,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest2) {
    iec104->setJsonConfig(protocol_config_broken2,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest3) {
    iec104->setJsonConfig(protocol_config_broken3,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest4) {
    iec104->setJsonConfig(protocol_config_broken4,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest5) {
    iec104->setJsonConfig(protocol_config_broken5,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest6) {
    iec104->setJsonConfig(protocol_config_broken6,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest7) {
    iec104->setJsonConfig(protocol_config_broken7,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest8) {
    iec104->setJsonConfig(protocol_config_broken8,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest9) {
    iec104->setJsonConfig(protocol_config_broken9,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest10) {
    iec104->setJsonConfig(protocol_config_broken10,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest11) {
    iec104->setJsonConfig(protocol_config_broken11,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest12) {
    iec104->setJsonConfig(protocol_config_broken12,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest13) {
    iec104->setJsonConfig(protocol_config_broken13,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest14) {
    iec104->setJsonConfig(protocol_config_broken14,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest15) {
    iec104->setJsonConfig(protocol_config_broken15,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest16) {
    iec104->setJsonConfig(protocol_config_broken16,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest17) {
    iec104->setJsonConfig(protocol_config_broken17,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest18) {
    iec104->setJsonConfig(protocol_config_broken18,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest19) {
    iec104->setJsonConfig(protocol_config_broken19,exchanged_data,tls_config);
}

TEST_F(ConfigTest, ConfigTest20) {
    iec104->setJsonConfig(protocol_config,exchanged_data,tls_config);
    iec104->setJsonConfig(protocol_config,exchanged_data_2,tls_config);
}

TEST_F(ConfigTest, ConfigTest21) {
    iec104->setJsonConfig(protocol_config,exchanged_data,tls_config_broken1);
}

TEST_F(ConfigTest, ConfigTest22) {
    iec104->setJsonConfig(protocol_config,exchanged_data_broken1,tls_config);
}

TEST_F(ConfigTest, ConfigTest23) {
    iec104->setJsonConfig(protocol_config,exchanged_data,tls_config);
}


// TEST_F(ConfigTest, ConfigTest1)
// {
//     asduHandlerCalled = 0;
//     clockSyncHandlerCalled = 0;
//     lastConnection = NULL;
//     ingestCallbackCalled = 0;

//     CS104_Slave slave = CS104_Slave_create(15, 15);

//     CS104_Slave_setLocalPort(slave, TEST_PORT);

//     CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
//     CS104_Slave_setASDUHandler(slave, asduHandler, this);

//     CS104_Slave_start(slave);

//     CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

//     startIEC104();

//     Thread_sleep(500);

//     // quality update for measurement data points
//     ASSERT_EQ(3, ingestCallbackCalled);

//     PLUGIN_PARAMETER* params[9];

//     PLUGIN_PARAMETER type = {"type", "C_SC_NA_1"};
//     params[0] = &type;

//     PLUGIN_PARAMETER ca = {"ca", "41025"};
//     params[1] = &ca;

//     // ioa
//     PLUGIN_PARAMETER ioa = {"ioa", "2000"};
//     params[2] = &ioa;

//     // Third value
//     PLUGIN_PARAMETER value = {"value", "1"};
//     params[8] = &value;

//     // Third value
//     PLUGIN_PARAMETER select = {"se", "0"};
//     params[5] = &select;
//     // PLUGIN_PARAMETER ts = {"ts", "2515224"};
//     // params[7] = &ts;

//     // bool operationResult = iec104->operation("IEC104Command", 9, params);
//     printf("Alo\n");
//     bool operationResult = ConfigTest::operation("IEC104Command", 9, params);
//     printf("Alo\n");
//     // bool checkType = ConfigTest::checkTypeCommand(41025,2000,1,0,0,C_SC_TA_1);

//     ASSERT_TRUE(operationResult);

//     ASSERT_TRUE(operationResult);

//     Thread_sleep(500);

//     ASSERT_EQ(1, asduHandlerCalled);

//     CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),false, CS101_COT_ACTIVATION_TERMINATION,lastOA, 41025, false, false);

//     CP56Time2a timestamp = CP56Time2a_createFromMsTimestamp(NULL, Hal_getTimeInMs());

//     InformationObject io = (InformationObject)SingleCommand_create(NULL, 2000, 1, false, 0);

//     CS101_ASDU_addInformationObject(ctAsdu, io);

//     IMasterConnection_sendASDU(lastConnection, ctAsdu);

//     InformationObject_destroy(io);

//     free(timestamp);

//     CS101_ASDU_destroy(ctAsdu);

//     ASSERT_EQ(10, lastOA);

//     Thread_sleep(500);

//     // expect ingest callback called two more times:
//     //  1. ACT_CON for single command
//     //  2. ACT_TERM for single command
//     ASSERT_EQ(3 + 2, ingestCallbackCalled);

//     CS104_Slave_stop(slave);

//     CS104_Slave_destroy(slave);
// }