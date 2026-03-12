# quantlearn

**quantlearn** is a research-oriented project focused on learning **Linear Temporal Logic (LTL) formulae** from sample traces. The project aims to infer formal temporal specifications that explain observed system behaviors, supporting applications in formal verification, runtime monitoring, and automated specification mining.

---

## Overview

Given a set of positive and/or negative execution samples, `quantlearn` provides mechanisms to synthesize LTL formulae that are consistent with the observed data. This enables:

- Automatic discovery of temporal properties  
- Support for formal methods and verification workflows  
- Experimentation with learning-based approaches to temporal logic  

---

## Building from Source

### Prerequisites

Make sure the following tools are installed on your system:

- GNU Autotools (`autoconf`, `automake`, `autoreconf`)
- A compatible C/C++ compiler (e.g., `gcc`, `clang`)
- `make`

---

### Compilation Steps

1. Move into the `src` directory:

   ```bash
   cd src

2. Generate the configuration scripts using Autotools:
   ```bash
   autoreconf --install
3. Configure the project and compile the source code:
   ```bash
   ./configure
   make
