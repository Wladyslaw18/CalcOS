# Statistics Reference

All statistical functions accept a variable number of comma-separated arguments.

| Function | Description | Example |
|---|---|---|
| `mean(...)` | Arithmetic mean | `mean(1,2,3,4,5)` = 3.0 |
| `median(...)` | Median (NaN-safe sort) | `median(10,20,30)` = 20.0 |
| `var(...)` | Population variance | `var(1,2,3)` = 0.667 |
| `std(...)` | Standard deviation | `std(1,2,3)` = 0.816 |
| `cov(x1,y1, x2,y2, ...)` | Covariance (paired) | `cov(1,2, 3,4, 5,6)` |
| `corr(x1,y1, x2,y2, ...)` | Pearson correlation | `corr(1,2, 3,4, 5,6)` = 1.0 |
| `min(...)` | Minimum value | `min(5,3,8,1)` = 1.0 |
| `max(...)` | Maximum value | `max(5,3,8,1)` = 8.0 |

## NaN Handling

If any argument is `NaN` (e.g. from `sqrt(-1)` in real mode), NaN values are
sorted to the right of the array before computing statistics. The mean/median
operate only on the non-NaN prefix.

## Implementation Notes

- Sorting uses an in-place quicksort on a stack-allocated array (max 256 values)
- No heap allocation - arguments are copied to a local double array on the stack
- For `cov`/`corr`, arguments are interleaved pairs: x1,y1,x2,y2,...
