# -*- coding: utf-8 -*-
"""QGC Complete Chinese Translation Script"""
import xml.etree.ElementTree as ET
import re, json, sys, os

def load_translations():
    """Load translation dictionary from JSON file."""
    with open(os.path.join(os.path.dirname(__file__), 'trans_dict.json'), 'r', encoding='utf-8') as f:
        return json.load(f)

def translate_ts_file(ts_path, trans_dict):
    """Apply translations to a .ts file."""
    # Parse preserving structure
    tree = ET.parse(ts_path)
    root = tree.getroot()
    
    translated = 0
    total_unfinished = 0
    
    for context in root.findall('context'):
        for msg in context.findall('message'):
            trans_elem = msg.find('translation')
            if trans_elem is not None and trans_elem.get('type') == 'unfinished':
                total_unfinished += 1
                source = msg.find('source').text or ''
                
                if source in trans_dict:
                    trans_elem.text = trans_dict[source]
                    del trans_elem.attrib['type']
                    translated += 1
    
    # Write back with proper XML declaration
    tree.write(ts_path, encoding='utf-8', xml_declaration=True)
    
    # Fix: ET doesn't write DOCTYPE, we need to add it back
    with open(ts_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Add DOCTYPE after XML declaration
    content = content.replace(
        "?>\n<TS",
        "?>\n<!DOCTYPE TS>\n<TS"
    )
    
    with open(ts_path, 'w', encoding='utf-8') as f:
        f.write(content)
    
    return translated, total_unfinished

def main():
    trans_dict = load_translations()
    print(f"Loaded {len(trans_dict)} translations")
    
    base = r'E:\drone\QGC-debug\qgroundcontrol\translations'
    
    for fname in ['qgc_source_zh_CN.ts', 'qgc_json_zh_CN.ts']:
        path = os.path.join(base, fname)
        translated, total = translate_ts_file(path, trans_dict)
        print(f"{fname}: translated {translated}/{total} strings")

if __name__ == '__main__':
    main()
