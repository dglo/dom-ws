/^- Package:/ { 
    package = substr($0, 12); hasStats=0; 
}

/^Stats:$/ { 
    hasStats = 1; deps=""; 
}

/^Depends Upon:$/ { 
    doDeps = 1;
    deps = package;
}

/^Used By:$/ { 
    doDeps = 0; 
    print jar " " package " " deps;
}

/    [a-zA-Z]/ {
    if (doDeps && match($0, "java\.")!=5 && match($0, "sun\.")!=5 && 
	match($0, "com.sun")!=5) {
	deps = deps " " substr($0, 5);
    }
}
