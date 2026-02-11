# Known Issues

These are __some__ of the known issues yet to be solved: whether by mere update on a source file or one that needs more debugging, listed from the most significant.

## Persistent setup cycle

The maximum PCR cycle on the Setup page is persistent. After being modified (e.g. from the default 55 in `new_experiment` to 45), one would expect the Raw Data page and the Amplification Curve page to show corresponding changes, but apparently otherwise is the case. And after one flies back to the Setup page, it states a persistent default number for the cycle (55 in this case). 

The issue is apparent for all the other `resources/experiments/` files, thus it must be about their handler classes (e.g., `DataManager`).

## Memory spike

Memory usage of the app spikes to 6-7x of its initial idling usage (about 150 MemB) after PWM toggling of the LED slider and/or after multiple PCR cycle modifications and/or after a heap of state jumps on the GUI. This is why it is crucial to have at least 4 GB of RAM for deployment for as long as this issue remains unsolved. 

## No automated stop on Timer mode

The `HardwareController::onSensorTimer` method itself is sufficient for timer (and hence sensor) control (i.e., for the latter's automated starting and stopping). The sensor stops once `HardwareController::stopSensorReading()` is called, and that time is the instance `HardwareController::m_maxPcrCycle` is equal to the amount stored in `DataManager::m_maxCycle`. Nevertheless, the STOP button on the GUI does not automatically switch to RUN (and permits further user operations) at the same instance, which by design it must.

## Save data

`DataManager::save_data` is not working on the Pi.

## Further improvements

These are the immediate future improvements suggested by the previous examiners and by our standards and evaluations.

1. Adding feedback on the Run page so the user can tell how much time has elapsed, whether the PCR detection is still going, or if the system froze.
2. Adding bidirectional serial communication with the DMF in place of the current unidirectional so that the DMF can receive commands from the Pi as well.
3. In addition to point 2, complete DMF control on the GUI, instead of the currently limited detection-system-only.
