HD Speccys
==========

TODO
----

- There is a bug with the interrupts that I don't understand. 
  - FIQ now seems only to fail for the border IRQ. Using no FIQ also seems to work.
  - ZxINT = FIQ, ZxBORDER = IRQ  ZxTAPE(timer) = IRQ + Network + OpenGL ==> OK! Loads Back2Skool game OK under stress from 100ms pings
  - Maybe check out the code for the IRQ controller. It seems to be an issue with having more than 2 IRQs.
    This makes no sense at all, but needs to be investigated. How it interacts with WiFi/Network and OpenGL also needs checking
- Run 'npm install' in `tools/flashy`


