#ifndef UNITTEST_HPP
#define UNITTEST_HPP

int Test_Position();
bool Test_Collision();
bool Test_InsideGrid();
bool Test_HoverStates();
bool Test_ComputeHMAC();
bool Test_hmacEquals();
bool Test_validateHMAC_Missing();
bool Test_validateHMAC_Correct();
bool Test_validateHMAC_BadTag();
bool Test_PairingSync();
bool Test_resetClientState();
bool Test_stopBroadcast_flag();
bool Test_VariableJitterSimulation();
bool Test_PacketLossSimulation();

#endif 