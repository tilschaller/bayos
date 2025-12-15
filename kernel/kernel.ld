OUTPUT_FORMAT(binary)

ENTRY(_start)

MEMORY {
  kernel (rwx) : ORIGIN = 0xFFFFFFFF80000000, LENGTH = 
}
