
# Attacks

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

# Latency

## ISR latency

With `MAX_TIMESTAMP = 6000`, which corresponds to 6000 ISR triggered, about 5 minutes on a standard connection.

#### Peripheral HR and `conn_rx`

Used in conjunction with the android app nRF Connect.

`Peripheral_HR` example **without** Sirocco, and `conn_rx`:
```
[00:05:36.155,181] <inf> srccanalysis: [conn_rx] (cycles) Average: 1717057/6000 min: 261 max: 427
[00:05:36.155,212] <inf> srccanalysis: [conn_rx] (us) Average: 26829/6000 min: 4 max: 6
[00:12:15.607,330] <inf> srccanalysis: [conn_rx] (cycles) Average: 1720156/6000 min: 260 max: 342
[00:12:15.607,360] <inf> srccanalysis: [conn_rx] (us) Average: 26877/6000 min: 4 max: 5
[00:17:28.823,791] <inf> srccanalysis: [conn_rx] (cycles) Average: 1721168/6000 min: 260 max: 342
[00:17:28.823,822] <inf> srccanalysis: [conn_rx] (us) Average: 26893/6000 min: 4 max: 5
```
- On average we count **286 cycles** => **4,47 us**.
- The first max `427` is observed constantly and corresponds to the start of the connection (hypth)
- Repeat 3 times the experiment and observe consistent values across the 3 runs

`Peripheral_HR` example for `conn_rx` **with** Sirocco and KNOB + BTLEJuice + InjectaBLE modules:
```
[00:05:25.431,091] <inf> srccanalysis: [conn_rx] (cycles) Average: 3969067/6000 min: 633 max: 822
[00:05:25.431,121] <inf> srccanalysis: [conn_rx] (us) Average: 62016/6000 min: 9 max: 12
[00:10:30.945,098] <inf> srccanalysis: [conn_rx] (cycles) Average: 3956334/6000 min: 657 max: 660
[00:10:30.945,129] <inf> srccanalysis: [conn_rx] (us) Average: 61817/6000 min: 10 max: 10
[00:15:36.020,355] <inf> srccanalysis: [conn_rx] (cycles) Average: 3962371/6000 min: 657 max: 733
[00:15:36.020,385] <inf> srccanalysis: [conn_rx] (us) Average: 61912/6000 min: 10 max: 11
[00:20:35.784,332] <inf> srccanalysis: [conn_rx] (cycles) Average: 3966971/6000 min: 657 max: 733
[00:20:35.784,362] <inf> srccanalysis: [conn_rx] (us) Average: 61983/6000 min: 10 max: 11
[00:25:37.108,367] <inf> srccanalysis: [conn_rx] (cycles) Average: 3968684/6000 min: 657 max: 734
[00:25:37.108,398] <inf> srccanalysis: [conn_rx] (us) Average: 62010/6000 min: 10 max: 11
```
- We observe the same behavior for the max on the first measure.
- On average we count **660 cycles** => **10,33 us**.
- It is an latency overhead of **130,77%** of cycles.

#### Central HR and `conn_rx`

Used with the example `sirocco_peripheral_hr`.

`Central_HR` example **without** Sirocco and `conn_rx`:
```
[00:05:07.698,425] <inf> srccanalysis: [conn_tx] (cycles) Average: 1382396/6000 min: 229 max: 231
[00:05:07.698,486] <inf> srccanalysis: [conn_tx] (us) Average: 21599/6000 min: 3 max: 3
[...]
[00:05:06.425,659] <inf> srccanalysis: [conn_tx] (cycles) Average: 1382397/6000 min: 229 max: 231
[00:05:06.425,720] <inf> srccanalysis: [conn_tx] (us) Average: 21599/6000 min: 3 max: 3
```
- On average we count **230 cycles** => **3,60 us**.

`Central_HR` example for `conn_rx` **with** Sirocco and KNOB + BTLEJack modules:
```
[00:05:09.303,466] <inf> srccanalysis: [conn_tx] (cycles) Average: 2716808/6000 min: 444 max: 454
[00:05:09.303,497] <inf> srccanalysis: [conn_tx] (us) Average: 42450/6000 min: 6 max: 7
[00:05:09.353,698] <inf> srccanalysis: [conn_rx] (cycles) Average: 3356493/6000 min: 470 max: 798
[00:05:09.353,729] <inf> srccanalysis: [conn_rx] (us) Average: 52445/6000 min: 7 max: 12
[00:10:09.303,466] <inf> srccanalysis: [conn_tx] (cycles) Average: 2717332/6000 min: 452 max: 454
[00:10:09.303,497] <inf> srccanalysis: [conn_tx] (us) Average: 42458/6000 min: 7 max: 7
[00:10:09.403,717] <inf> srccanalysis: [conn_rx] (cycles) Average: 3353870/6000 min: 471 max: 559
[00:10:09.403,717] <inf> srccanalysis: [conn_rx] (us) Average: 52404/6000 min: 7 max: 8
[00:15:09.303,466] <inf> srccanalysis: [conn_tx] (cycles) Average: 2717333/6000 min: 452 max: 454
[00:15:09.303,497] <inf> srccanalysis: [conn_tx] (us) Average: 42458/6000 min: 7 max: 7
[00:15:09.453,704] <inf> srccanalysis: [conn_rx] (cycles) Average: 3353869/6000 min: 470 max: 559
[00:15:09.453,735] <inf> srccanalysis: [conn_rx] (us) Average: 52404/6000 min: 7 max: 8
```
- We observed 2 types of runs
- On average we count **452 or 559 cycles** => **8,74 us**.
- It is an latency overhead of **96,52 and 143,04%** of cycles.

#### Notes on KNOB module

We believe the major part of the latency overhead comes from the copy of the payload needed for KNOB.
Therefore we rerun the experiments by disabling KNOB:

`Peripheral_HR` example for `conn_rx` **with** Sirocco and BTLEJuice + InjectaBLE modules:
```
[00:05:10.944,091] <inf> srccanalysis: [conn_tx] (cycles) Average: 2711993/6000 min: 443 max: 453
[00:05:10.944,122] <inf> srccanalysis: [conn_tx] (us) Average: 42374/6000 min: 6 max: 7
[00:05:11.144,317] <inf> srccanalysis: [conn_rx] (cycles) Average: 2743898/6000 min: 374 max: 655
[00:05:11.144,348] <inf> srccanalysis: [conn_rx] (us) Average: 42873/6000 min: 5 max: 10
[00:10:10.944,091] <inf> srccanalysis: [conn_tx] (cycles) Average: 2712000/6000 min: 452 max: 452
[00:10:10.944,122] <inf> srccanalysis: [conn_tx] (us) Average: 42375/6000 min: 7 max: 7
[00:10:11.344,329] <inf> srccanalysis: [conn_rx] (cycles) Average: 2741875/6000 min: 374 max: 457
[00:10:11.344,329] <inf> srccanalysis: [conn_rx] (us) Average: 42841/6000 min: 5 max: 7
```
- Disabling the KNOB module results instead in a **457 cycles**, a similar order of magnitude therefore.

`Central_HR` example for `conn_rx` **with** Sirocco and BTLEJack module:
```
```
- Disabling the KNOB module results instead in a latency overhead increased of ** cycles**.

#### Central HR and `scan_rx`

Used with the example `broadcaster`.

`Central_HR` example **without** Sirocco and `scan_rx`:
```
[00:07:29.640,655] <inf> srccanalysis: [scan_rx] (cycles) Average: 1858538/6000 min: 142 max: 340
[00:07:29.640,716] <inf> srccanalysis: [scan_rx] (us) Average: 29039/6000 min: 2 max: 5
[00:14:59.210,357] <inf> srccanalysis: [scan_rx] (cycles) Average: 1866793/6000 min: 142 max: 340
[00:14:59.210,418] <inf> srccanalysis: [scan_rx] (us) Average: 29168/6000 min: 2 max: 5
[00:22:21.408,355] <inf> srccanalysis: [scan_rx] (cycles) Average: 1860627/6000 min: 142 max: 340
[00:22:21.408,386] <inf> srccanalysis: [scan_rx] (us) Average: 29072/6000 min: 2 max: 5
```
- On average we count **311 cycles** => **4,86 us**.

`Central_HR` example for `scan_rx` **with** Sirocco and GATTacker module:
```
[00:07:27.349,609] <inf> srccanalysis: [scan_rx] (cycles) Average: 2167758/6000 min: 181 max: 600
[00:07:27.349,670] <inf> srccanalysis: [scan_rx] (us) Average: 33871/6000 min: 2 max: 9
[00:14:22.927,947] <inf> srccanalysis: [scan_rx] (cycles) Average: 2201354/6000 min: 180 max: 600
[00:14:22.928,009] <inf> srccanalysis: [scan_rx] (us) Average: 34396/6000 min: 2 max: 9
[00:21:46.619,506] <inf> srccanalysis: [scan_rx] (cycles) Average: 2147479/6000 min: 181 max: 600
[00:21:46.619,537] <inf> srccanalysis: [scan_rx] (us) Average: 33554/6000 min: 2 max: 9
```
- On average we count **361 cycles** => **5,73 us**.
- It is an latency overhead of **16,08%** of cycles.


#### Observer and `scan_rx`

Used with the example `broadcaster`.

`Observer` example **without** Sirocco and `scan_rx`:
```
[00:03:41.665,954] <inf> srccanalysis: [scan_rx] (cycles) Average: 1520921/6000 min: 149 max: 257
[00:03:41.665,985] <inf> srccanalysis: [scan_rx] (us) Average: 23764/6000 min: 2 max: 4
[00:07:22.637,084] <inf> srccanalysis: [scan_rx] (cycles) Average: 1519872/6000 min: 157 max: 257
[00:07:22.637,145] <inf> srccanalysis: [scan_rx] (us) Average: 23748/6000 min: 2 max: 4
[00:10:57.752,868] <inf> srccanalysis: [scan_rx] (cycles) Average: 1510726/6000 min: 149 max: 258
[00:10:57.752,899] <inf> srccanalysis: [scan_rx] (us) Average: 23605/6000 min: 2 max: 4
[00:14:37.717,102] <inf> srccanalysis: [scan_rx] (cycles) Average: 1515932/6000 min: 149 max: 258
[00:14:37.717,132] <inf> srccanalysis: [scan_rx] (us) Average: 23686/6000 min: 2 max: 4
```
- On average we count **253 cycles** => **3,96 us**.

`Observer` example **with** Sirocco and `scan_rx`:
```
[00:03:38.101,135] <inf> srccanalysis: [scan_rx] (cycles) Average: 1894805/6000 min: 208 max: 530
[00:03:38.101,165] <inf> srccanalysis: [scan_rx] (us) Average: 29606/6000 min: 3 max: 8
[00:07:11.180,664] <inf> srccanalysis: [scan_rx] (cycles) Average: 1916894/6000 min: 208 max: 530
[00:07:11.180,694] <inf> srccanalysis: [scan_rx] (us) Average: 29951/6000 min: 3 max: 8
[00:10:35.162,048] <inf> srccanalysis: [scan_rx] (cycles) Average: 1965920/6000 min: 206 max: 535
[00:10:35.162,078] <inf> srccanalysis: [scan_rx] (us) Average: 30717/6000 min: 3 max: 8
[00:14:09.614,227] <inf> srccanalysis: [scan_rx] (cycles) Average: 1904751/6000 min: 204 max: 528
[00:14:09.614,288] <inf> srccanalysis: [scan_rx] (us) Average: 29761/6000 min: 3 max: 8
```
- On average we count **328 cycles** => **5,12 us**.
- It is an latency overhead of **29,64%** of cycles.




