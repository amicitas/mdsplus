public fun Dt216Init(IN _board, IN _activeChans, IN _trigSrc, IN _clockSource, IN _clockFreq, IN _preTrig, IN _postTrig)
{
  _mask='';
  for (_i=0; _i<16; _i++) {
    if (_i < _activeChans) {
      _mask = _mask//'1';
    } else {
      _mask = _mask//'0';
    }
  }
  Dt200WriteMaster(_board, "setChannelMask "//_mask);
  if (_clockSource == 'MASTER') {
    Dt200WriteMaster(_board, "setInternalClock "//long(_clockFreq)//" DO1");
/*    Dt200WriteMaster(_board, "set.route d1 in fpga out pxi", 1); */
    Dt200WriteMaster(_board, "-- setDIO -1-----"); 
  } else { 
    if (_clockSource == 'INT') {
      Dt200WriteMaster(_board, "setInternalClock "//LONG(_clockFreq));
    } else {
      Dt200WriteMaster(_board, "setExternalClock "//_clockSource);
      if (_clockFreq > 0) {
        write (*, "don't forget about the clock divider");
      }
    }
  }
  if (_preTrig == 0) {
     write(*, "_preTrig is zero");
     Dt200WriteMaster(_board, 'set.trig '//_trigSrc//' rising', 1);
     Dt200WriteMaster(_board, 'set.event0 none', 1);
     Dt200WriteMaster(_board, 'setMode GATED_TRANSIENT '//_postTrig);
  } else {
     write(*, "_preTrig is NOT zero");
     Dt200WriteMaster(_board, 'set.trig none', 1);
     Dt200WriteMaster(_board, 'set.event0 '//_trigSrc//' rising', 1);
     Dt200WriteMaster(_board, 'setModeTriggeredContinuous '//_preTrig//' '//_postTrig);
  }
/*  Dt200WriteMaster(_board, "set.pre_post_mode "//_preTrig//" "//_postTrig//" "//_trigSrc//" rising", 1); */
  Dt200WriteMaster(_board, "setArm");
  write(*, "Dt216Init is done");
  return(1);
}
