#!/bin/bash

dom=10a

se ${dom} < dump.se | sed 's/^DUMP //1' | tr ' ' '\n' | tr '=' '\t'

