# GreenHashTree
Supercool sub-zero green in memory key-value effecient hash-tree.  
The best around.  
NOT totally ordered. Only orders the values within the subtree the values ends up at from hashing. Is this a problem?  

1: Find the number of cores in the system.  
2: Use a low-cost hash to somewhat uniformly distribute data over the cores.  
3: Put the key-value pair in the correct CPU's tree.  
