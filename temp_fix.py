# -*- coding: utf-8 -*-
import codecs

path = r"D:\Workspaces\Work\GraduationProject\NetDisk\src\qml\Common\SearchBox.qml"

with open(path, 'rb') as f:
    raw = f.read()

print("First 500 bytes (repr):")
print(repr(raw[:500]))