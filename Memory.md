# Memory explained

## Physical memory map
| Address range   | Usage            |
|:---------------:|------------------|
| 0      - 2 MiB  | Reserved         |
| 2 MiB  - 4 MiB  | Kernel binary    |
| 4 MiB  - 8 MiB  | Kbrk/kmalloc     |
| 8 MiB  - 16 MiB | Kbrk grow/unused |
| 16 MiB -        | Unused           |

## Virtual memory map
| Address range     | Usage            |
|:-----------------:|------------------|
| 0       - 2 MiB   | Reserved         |
| 2 MiB   - 4 MiB   | Kernel binary    |
| 4 MiB   - 16 MiB  | Kbrk/kmalloc     |
| ...               | ...              |
| 3 GiB   - +16 MiB | All of the above |
| 3,5 GiB - 4 GiB   | Vmalloc          |

## Notes
Kbrk grows linearly if *physical memory* is available, because there is a direct mapping of the address range [0; 16 MiB] to [3 GiB; 3 GiB + 16 MiB]. Hence, vmalloc will try to avoid using memory that kbrk could need by looking for physical frames starting at 16 MiB. If no frame is available in that range though, it will eat the space of kbrk.
