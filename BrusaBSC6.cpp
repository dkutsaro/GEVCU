/*
 * BrusaBSC6.cpp
 *
 *
 Copyright (c) 2013 Collin Kidder, Michael Neuweiler, Charles Galpin

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#include "BrusaBSC6.h"

/*
 * Constructor
 */
BrusaBSC6::BrusaBSC6() : DcDcConverter()
{
    canHandlerEv = CanHandler::getInstanceEV();
    prefsHandler = new PrefHandler(BRUSA_BSC6);
    commonName = "Brusa BSC6 DC-DC Converter";

    hvVoltage = 0;
    lvVoltage = 0;
    hvCurrent = 250;
    lvCurrent = 280;
    mode = 0;
    lvCurrentAvailable = 0;
    maxTemperature = 0;
    temperatureBuckBoostSwitch1 = 0;
    temperatureBuckBoostSwitch2 = 0;
    temperatureHvTrafostageSwitch1 = 0;
    temperatureHvTrafostageSwitch2 = 0;
    temperatureLvTrafostageSwitch1 = 0;
    temperatureLvTrafostageSwitch2 = 0;
    temperatureTransformerCoil1 = 0;
    temperatureTransformerCoil2 = 0;
    internal12VSupplyVoltage = 0;
    lsActualVoltage = 0;
    lsActualCurrent = 6000;
    lsCommandedCurrent = 6000;
    internalOperationState = 0;
}

/*
 * Setup the device if it is enabled in configuration.
 */
void BrusaBSC6::setup()
{
    tickHandler->detach(this);

    Logger::info("add device: %s (id: %X, %X)", commonName, BRUSA_BSC6, this);

    loadConfiguration();
    Device::setup(); // call parent

    // register ourselves as observer of 0x26a-0x26f can frames
    canHandlerEv->attach(this, CAN_MASKED_ID, CAN_MASK, false);

    tickHandler->attach(this, CFG_TICK_INTERVAL_DCDC_BSC6);
}

/*
 * Process event from the tick handler.
 */
void BrusaBSC6::handleTick()
{
    Device::handleTick(); // call parent

    sendCommand();
    sendLimits();
}

/*
 * Send BSC6COM message to the DCDC converter.
 *
 * The message is used to set the operation mode, enable the converter
 * and set the voltage limits.
 */
void BrusaBSC6::sendCommand()
{
    BrusaBSC6Configuration *config = (BrusaBSC6Configuration *) getConfiguration();
    prepareOutputFrame(CAN_ID_COMMAND);

    if ((status->getSystemState() == Status::running)
            && systemIO->getEnableInput()) {
        outputFrame.data.bytes[0] |= enable;
    }
    if (config->boostMode) {
        outputFrame.data.bytes[0] |= boostMode;
    }
    if (config->debugMode) {
        outputFrame.data.bytes[0] |= debugMode;
    }

    outputFrame.data.bytes[1] = config->lowVoltage; // 8-16V in 0.1V, offset = 0V
    outputFrame.data.bytes[2] = config->highVoltage -170; // 190-425V in 1V, offset = 170V

//    canHandlerEv->sendFrame(outputFrame);
}

/*
 * Send BSC6LIM message to DCDC converter.
 *
 * This message controls the electrical limits in the converter.
 */
void BrusaBSC6::sendLimits()
{
    BrusaBSC6Configuration *config = (BrusaBSC6Configuration *) getConfiguration();
    prepareOutputFrame(CAN_ID_LIMIT);

    outputFrame.data.bytes[0] = config->hvUndervoltageLimit - 170;
    outputFrame.data.bytes[1] = config->lvBuckModeCurrentLimit;
    outputFrame.data.bytes[2] = config->hvBuckModeCurrentLimit;
    outputFrame.data.bytes[3] = config->lvUndervoltageLimit;
    outputFrame.data.bytes[4] = config->lvBoostModeCurrentLinit;
    outputFrame.data.bytes[5] = config->hvBoostModeCurrentLimit;

//    canHandlerEv->sendFrame(outputFrame);
}

/*
 * Prepare the CAN transmit frame.
 * Re-sets all parameters in the re-used frame.
 */
void BrusaBSC6::prepareOutputFrame(uint32_t id)
{
    outputFrame.length = 8;
    outputFrame.id = id;
    outputFrame.extended = 0;
    outputFrame.rtr = 0;

    outputFrame.data.bytes[0] = 0;
    outputFrame.data.bytes[1] = 0;
    outputFrame.data.bytes[2] = 0;
    outputFrame.data.bytes[3] = 0;
    outputFrame.data.bytes[4] = 0;
    outputFrame.data.bytes[5] = 0;
    outputFrame.data.bytes[6] = 0;
    outputFrame.data.bytes[7] = 0;
}

/*
 * Processes an event from the CanHandler.
 *
 * In case a CAN message was received which pass the masking and id filtering,
 * this method is called. Depending on the ID of the CAN message, the data of
 * the incoming message is processed.
 */
void BrusaBSC6::handleCanFrame(CAN_FRAME *frame)
{
    switch (frame->id) {
        case CAN_ID_VALUES_1:
            processValues1(frame->data.bytes);
            break;

        case CAN_ID_VALUES_2:
            processValues2(frame->data.bytes);
            break;

        case CAN_ID_DEBUG_1:
            processDebug1(frame->data.bytes);
            break;

        case CAN_ID_DEBUG_2:
            processDebug2(frame->data.bytes);
            break;

//        default:
//            Logger::warn(BRUSA_BSC6, "received unknown frame id %X", frame->id);
    }
}

/*
 * Process a BSC6VAL1 message which was received from the converter.
 *
 * This message provides the general status of the converter as well as
 * available high/low voltage current and voltage.
 */
void BrusaBSC6::processValues1(uint8_t data[])
{
    bitfield = (uint32_t)data[7] & 0x0F;

    if (bitfield & running) {
        //TODO: interaction with system state ?
    }
    if (bitfield & ready) {
        //TODO: interaction with system state ?
    }
    if (bitfield & automatic) {
        //TODO: interaction with system state ?
    }

    hvVoltage = (uint16_t)(data[1] | (data[0] << 8));
    lvVoltage = (uint8_t)data[2];
    hvCurrent = (int16_t)(data[4] | (data[3] << 8)) - 250;
    lvCurrent = (int16_t)(data[6] | (data[5] << 8)) - 280;
    mode = (data[7] & 0xF0) >> 4;

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_BSC6, "status bitfield: %X, HV: %fV, %fA, LV: %fV, %dA, mode %d", bitfield, (float) hvVoltage / 10.0F, (float) hvCurrent / 10.0F, (float) lvVoltage / 10.0F, lvCurrent, mode);
    }
}

/*
 * Process a BSC6VAL2 message which was received from the converter.
 *
 * This message provides (critical) errors.
 */
void BrusaBSC6::processValues2(uint8_t data[])
{
    lvCurrentAvailable = (uint8_t)data[0];
    maxTemperature = (uint8_t)data[1];

    bitfield = (uint32_t)((data[3] << 0) | (data[2] << 8) | (data[4] << 16));
    // TODO: react on various bitfields if set ?
//    lowVoltageUndervoltage
//    lowVoltageOvervoltage
//    highVoltageUndervoltage
//    highVoltageOvervoltage
//    internalSupply
//    temperatureSensor
//    trafoStartup
//    overTemperature
//    highVoltageFuse
//    lowVoltageFuse
//    currentSensorLowSide
//    currentDeviation
//    interLock
//    internalSupply12V
//    internalSupply6V
//    voltageDeviation
//    invalidValue
//    commandMessageLost
//    limitMessageLost
//    crcErrorNVSRAM
//    brokenTemperatureSensor

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_BSC6, "error bitfield: %X, LV current avail: %dA, maximum Temperature: %d�C", bitfield, lvCurrentAvailable, maxTemperature);
    }
}

/*
 * Process a BSC6DBG1 message which was received from the converter.
 *
 * This message provides various temperature readings.
 */
void BrusaBSC6::processDebug1(uint8_t data[])
{
    temperatureBuckBoostSwitch1 = (uint8_t)data[0];
    temperatureBuckBoostSwitch2 = (uint8_t)data[1];
    temperatureHvTrafostageSwitch1 = (uint8_t)data[2];
    temperatureHvTrafostageSwitch2 = (uint8_t)data[3];
    temperatureLvTrafostageSwitch1 = (uint8_t)data[4];
    temperatureLvTrafostageSwitch2 = (uint8_t)data[5];
    temperatureTransformerCoil1 = (uint8_t)data[6];
    temperatureTransformerCoil2 = (uint8_t)data[7];

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_BSC6, "Temp buck/boost switch 1: %d�C, switch 2: %d�C", temperatureBuckBoostSwitch1, temperatureBuckBoostSwitch2);
        Logger::debug(BRUSA_BSC6, "Temp HV trafostage switch 1: %d�C, switch 2: %d�C", temperatureHvTrafostageSwitch1, temperatureHvTrafostageSwitch2);
        Logger::debug(BRUSA_BSC6, "Temp LV trafostage switch 1: %d�C, switch 2: %d�C", temperatureLvTrafostageSwitch1, temperatureLvTrafostageSwitch2);
        Logger::debug(BRUSA_BSC6, "Temp transformer coil 1: %d�C, coil 2: %d�C", temperatureTransformerCoil1, temperatureTransformerCoil2);
    }
}

/*
 * Process a BSC6DBG2 message which was received from the converter.
 *
 * This message provides various temperature readings.
 */
void BrusaBSC6::processDebug2(uint8_t data[])
{
    internal12VSupplyVoltage = (uint8_t)data[0];
    lsActualVoltage = (uint8_t)data[1];
    lsActualCurrent = (int16_t)data[2] - 6000;
    lsCommandedCurrent = (int16_t)data[3] - 6000;
    internalOperationState = (uint8_t)data[4];

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_BSC6, "internal 12V supply: %fV, LS actual voltage: %fV", (float) internal12VSupplyVoltage / 10.0F, (float) lsActualVoltage / 10.0F);
        Logger::debug(BRUSA_BSC6, "LS actual current: %fA, LS commanded current: %fA", (float) internal12VSupplyVoltage / 200.0F, (float) lsActualVoltage / 200.0F);
        Logger::debug(BRUSA_BSC6, "internal power state: %d", internalOperationState);
    }
}

/*
 * Return the device type
 */
DeviceType BrusaBSC6::getType()
{
    return (DEVICE_DCDC);
}

/*
 * Return the device id of this device
 */
DeviceId BrusaBSC6::getId()
{
    return BRUSA_BSC6;
}

/*
 * Expose the tick interval of this controller
 */
uint32_t BrusaBSC6::getTickInterval()
{
    return CFG_TICK_INTERVAL_DCDC_BSC6;
}

/*
 * Load configuration data from EEPROM.
 *
 * If not available or the checksum is invalid, default values are chosen.
 */
void BrusaBSC6::loadConfiguration()
{
    BrusaBSC6Configuration *config = (BrusaBSC6Configuration *) getConfiguration();

    if (!config) { // as lowest sub-class make sure we have a config object
        config = new BrusaBSC6Configuration();
        setConfiguration(config);
    }

    Device::loadConfiguration(); // call parent

#ifdef USE_HARD_CODED

    if (false) {
#else
    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
        uint8_t temp;
        Logger::debug(BRUSA_BSC6, (char *) Constants::validChecksum);

        prefsHandler->read(DCDC_BOOST_MODE, &temp);
        config->boostMode = (temp != 0);
        prefsHandler->read(DCDC_DEBUG_MODE, &temp);
        config->debugMode = (temp != 0);
        prefsHandler->read(DCDC_LOW_VOLTAGE, &config->lowVoltage);
        prefsHandler->read(DCDC_HIGH_VOLTAGE, &config->highVoltage);
        prefsHandler->read(DCDC_HV_UNDERVOLTAGE_LIMIT, &config->hvUndervoltageLimit);
        prefsHandler->read(DCDC_LV_BUCK_CURRENT_LIMIT, &config->lvBuckModeCurrentLimit);
        prefsHandler->read(DCDC_HV_BUCK_CURRENT_LIMIT, &config->hvBuckModeCurrentLimit);
        prefsHandler->read(DCDC_LV_UNDERVOLTAGE_LIMIT, &config->lvUndervoltageLimit);
        prefsHandler->read(DCDC_LV_BOOST_CURRENT_LIMIT, &config->lvBoostModeCurrentLinit);
        prefsHandler->read(DCDC_HV_BOOST_CURRENT_LIMIT, &config->hvBoostModeCurrentLimit);
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        Logger::warn(BRUSA_BSC6, (char *) Constants::invalidChecksum);
        config->boostMode = false;
        config->debugMode = false;
        config->lowVoltage = 120;
        config->highVoltage = 20;
        config->hvUndervoltageLimit = 0;
        config->lvBuckModeCurrentLimit = 250;
        config->hvBuckModeCurrentLimit = 250;
        config->lvUndervoltageLimit = 100;
        config->lvBoostModeCurrentLinit = 250;
        config->hvBoostModeCurrentLimit = 250;
        saveConfiguration();
    }

    Logger::debug(BRUSA_BSC6, "Boost mode: %b, debug mode: %b, low Voltage: %fV, high Voltage: %dV", config->boostMode, config->debugMode, (float) config->lowVoltage / 10.0F, config->highVoltage);
    Logger::debug(BRUSA_BSC6, "HV undervoltage limit: %dV, LV buck mode current limit: %dA, HV buck mode current limit: %fA", config->hvUndervoltageLimit, config->lvBuckModeCurrentLimit, (float) config->hvBuckModeCurrentLimit / 10.0F);
    Logger::debug(BRUSA_BSC6, "LV undervoltage limit: %fV, LV boost mode current limit: %dA, HV boost mode current limit: %fA", (float) config->lvUndervoltageLimit / 10.0F, config->lvBoostModeCurrentLinit, (float) config->hvBoostModeCurrentLimit / 10.0F);
}

/*
 * Store the current configuration parameters to EEPROM.
 */
void BrusaBSC6::saveConfiguration()
{
    BrusaBSC6Configuration *config = (BrusaBSC6Configuration *) getConfiguration();

    Device::saveConfiguration(); // call parent

    prefsHandler->write(DCDC_BOOST_MODE, (uint8_t) (config->boostMode ? 1 : 0));
    prefsHandler->write(DCDC_DEBUG_MODE, (uint8_t) (config->debugMode ? 1 : 0));
    prefsHandler->write(DCDC_LOW_VOLTAGE, config->lowVoltage);
    prefsHandler->write(DCDC_HIGH_VOLTAGE, config->highVoltage);
    prefsHandler->write(DCDC_HV_UNDERVOLTAGE_LIMIT, config->hvUndervoltageLimit);
    prefsHandler->write(DCDC_LV_BUCK_CURRENT_LIMIT, config->lvBuckModeCurrentLimit);
    prefsHandler->write(DCDC_HV_BUCK_CURRENT_LIMIT, config->hvBuckModeCurrentLimit);
    prefsHandler->write(DCDC_LV_UNDERVOLTAGE_LIMIT, config->lvUndervoltageLimit);
    prefsHandler->write(DCDC_LV_BOOST_CURRENT_LIMIT, config->lvBoostModeCurrentLinit);
    prefsHandler->write(DCDC_HV_BOOST_CURRENT_LIMIT, config->hvBoostModeCurrentLimit);

    prefsHandler->saveChecksum();
}