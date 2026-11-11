import { access, presets } from 'zigbee-herdsman-converters/lib/exposes';
import { bind } from 'zigbee-herdsman-converters/lib/reporting';
import { Zcl } from 'zigbee-herdsman';
import { deviceAddCustomCluster } from 'zigbee-herdsman-converters/lib/modernExtend';

const CLUSTER_CUSTOM_ELECTRICITY = 0xFF00;
const ATTR_VALUE_IN = 0x0001;
const ATTR_VALUE_OUT = 0x0002;
const ATTR_VALUE_CURRENT = 0x0003;

const fzLocal = {
    energyleaf_digital: {
        cluster: 'custom_electricity',
        type: ['attributeReport', 'readResponse', 'read'],
        convert: (model, msg, publish, options, meta) => {
            const result = {};
                        if (msg.data.hasOwnProperty('value_in')) {
                            result.value_in = msg.data.value_in;
                        }
                        if (msg.data.hasOwnProperty('value_out')) {
                            result.value_out = msg.data.value_out;
                        }
                        if (msg.data.hasOwnProperty('value_current')) {
                            result.value_current = msg.data.value_current;
                        }
            return result;
        },
    },
};

export default [
    {
        zigbeeModel: ['A-Energyleaf-Digital'],
        model: 'A-Energyleaf-Digital',
        vendor: 'Energyleaf',
        description: 'Custom digital electricity meter',
        fromZigbee: [fzLocal.energyleaf_digital],
        toZigbee: [],
        exposes: [
            presets.numeric('value_in', access.STATE)
                .withUnit('kWh')
                .withDescription('Total energy consumed'),
            presets.numeric('value_out', access.STATE)
                .withUnit('kWh')
                .withDescription('Total energy generated'),
            presets.numeric('value_current', access.STATE)
                .withUnit('W')
                .withDescription('Current power'),
        ],
        extend: [
                deviceAddCustomCluster('custom_electricity', {
                        ID: CLUSTER_CUSTOM_ELECTRICITY,
                                attributes: {
                                        value_in: {
                                                ID: ATTR_VALUE_IN,
                                                type: Zcl.DataType.SINGLE_PREC
                                        },
                                        value_out: {
                                                ID: ATTR_VALUE_OUT,
                                                type: Zcl.DataType.SINGLE_PREC
                                        },
                                        value_current: {
                                                ID: ATTR_VALUE_CURRENT,
                                                type: Zcl.DataType.SINGLE_PREC
                                        },
                                },
                                commands: {},
                                commandsResponse: {},
                        }),
                ],
        configure: async (device, coordinatorEndpoint, logger) => {
        },
        endpoint: (device) => ({ default: 10 }),
    },
];