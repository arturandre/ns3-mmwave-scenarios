# ns-3mmWave Scenarios

In this report we described several experiments conducted using the ns-3 simulator and its module mmWave.

These experiments were motivated by some research questions, namely how to determine the best positioning of 5g radio base stations.

In this repository it made available the report in PDF format, the source code base used to produce the simulation scenarios and Jupyter notebooks with the analyses and graphs used in the report.

## Scenarios considered

Each scenario considers different physical structures (e.g. buildings), actors (e.g passerby) with varying moving speeds and different numbers and positions for 5g antennas (evolved Node B). 

### Scenario 1

In this scenario we created a container with 40 UE devices (a full bus) stopped near
5 eNBs (all of them 5g radio stations) located at the same spot. The rational behind
this arrangement is that perhaps with more eNBs the UEs could connect with distinct
eNBs and the overall SINR would be better. It turns out that by using the function
*AttachToClosestEnb* from the *MmWaveHelper* class all the UEs will connect to the same
eNB even when more are available if all eNBs are at the same position. In figure 1 we
plot the SINR from all the RNTIs in this scenario.

### Scenario 2

This scenario is exacltly like the last one except that instead of having 5 eNBs at the
same location now there is only one.

### Scenario 3

Following yet the investigations from the last scenarios now we tried to keep 40 UEs at a
fixed location and deployed two 5g eNBs each one at 150 meters away from the UEs but
in antipodal locations around the UEs. Still the conclusions and observations are quite
the same as the last two scenarios. So even in distinct locations, if the distance between
an UE and two eNBs the first eNB considered by the simulator will be the only one used.

### Scenario 4

In this scenario two real world buildings1 and a real world avenue are simulated together
with one UE crossing the avenue. There is one single eNB located behind one of the
buildings (from the point of view of the UE) so that during part of the simulation the
UE has direct line of sight to the eNB and in some parts it is blocked by the buildings.
Notice that the distance between the UE and the eNB changes very little during the
whole simulation. So the minimum distance is 50 meters and the maximum distance is
nearly 80 meters. Besides in this simulation in order to capture the RTT and the CWND
one remote host (i.e. a web-server over the internet) is inserted in the simulation, so that
this remote host sends data packages to each UE.

### Scenario 5

This scenario follows the same setting as the last one, except that the UE moves now
backwards (starting far away from the eNB and ending near it).

### Scenario 6

Since the behavior of the SINR, RTT and of the CWND, has proven to be quite consistent
for a simple scenario with one moving UE, one single eNB and two buildings, in this
scenario we tried to combine the two last ones. Now we have two UEs moving one
towards the other.

### Scenario 7

Now in addition to the last scenario we also inserted a second eNB (at the same distance
from the second UE as the first one with relation to the first UE) so that now each UE
will be getting close to an eNB at the same time that it is getting away from another.
With this scenario we want to check how the UEs interfere with each other and how is
the interaction of the eNBs in the presence of the obstacles.
