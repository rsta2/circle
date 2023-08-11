HD Speccys
==========

TODO
----

- There is a bug with the interrupts that I don't understand. 
  - There is a problem when the tape timer is on IRQ and the border interrupt occurs. Everything hangs.
  - If the tape timer is on FIQ then everything is fine!
  - Maybe check out the code for the IRQ controller. It seems to be an issue with having more than 2 IRQs.
    This makes no sense at all, but needs to be investigated. How it interacts with WiFi/Network and OpenGL also needs checking
- Run 'npm install' in `tools/flashy`


