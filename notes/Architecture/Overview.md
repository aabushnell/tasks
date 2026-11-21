
## Components
Right now the system exists as a single node. Eventually the design will expand beyond this but that level is currently undefined.
### Node Level
In terms of processes, the primary split is: (1) supervisor per node, (n) executors with n = (\#cores - 1).
Processes are assumed to be pinned to individual cores, without hyper-threading, and (for now) one thread per process.

The supervisor 

### Clients

