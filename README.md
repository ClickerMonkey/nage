# nage
This is not a game engine. What are you even doing here? Don't look at the code.

If you're still here and have some absurd feature request for something that's not even a game engine please file an Issue and watch it most likely get ignored. Unless you have good ideas. Unlikely.

### id::DenseMap vs std::map performance comparison:
```
testMapWrite:                      2.558989000s for writes: 262144
testDenseMapWrite:                 0.002186200s for writes: 262144
testDenseMapWrite (with area):     0.002879500s for writes: 262144
testMapUpdate:                     0.137761100s for updates: 524288
testDenseMapUpdate:                0.003221900s for updates: 524288
testDenseMapUpdate (with area):    0.006027700s for updates: 524288
testMapIteration:                  0.002035600s for iterations: 262144
testDenseMapIteration:             0.003494400s for iterations: 262144
testDenseMapIteration (with area): 0.006321900s for iterations: 262144
testMapRemove:                     0.210595600s for removes: 524288
testDenseMapRemove (no order):     0.003405400s for removes: 524288
testDenseMapRemove (ordered):      0.004247000s for removes: 524288
testDenseMapRemove (area, -order): 0.005781700s for removes: 524288
testDenseMapRemove (area, +order): 0.006930000s for removes: 524288
testVectorRemove (no order):       0.004498800s for removes: 524288
testVectorRemove (ordered):        0.020751800s for removes: 524288
```