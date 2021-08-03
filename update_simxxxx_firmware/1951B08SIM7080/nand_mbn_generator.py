#!/usr/bin/python
#===========================================================================

#  This script parses "partition_nand.xml" and creates numerous output files
#  specifically, partition_complete.xml, rawprogram_nand.xml
# REFERENCES

#  $Header: //components/rel/boot.xf/0.2/QcomPkg/Tools/storage/nandbootmbn/nand_mbn_generator.py#1 $
#  $DateTime: 2018/08/01 23:51:05 $ 
#  $Author: pwbldsvc $

# when          who     what, where, why 
# --------      ---     -------------------------------------------------------
# 2015-05-29    mj      Check-In
 
# Copyright (c) 2015
# Qualcomm Technologies Incorporated.
# All Rights Reserved.
# Qualcomm Confidential and Proprietary
# ===========================================================================*/

import pdb
import re
import sys,struct

from xml.etree import ElementTree as ET
#from elementtree.ElementTree import ElementTree
from xml.etree.ElementTree import Element, SubElement, Comment, tostring
from xml.dom import minidom

MIBIB_MAGIC1           = 0xFE569FAC
MIBIB_MAGIC2           = 0xCD7F127A
MIBIB_VERSION          = 0x4
mi_boot_info_block_format = '<IIII' # magic1, magic2, version, age


FLASH_PART_MAGIC1       = 0x55EE73AA
FLASH_PART_MAGIC2       = 0xE35EBDDB
FLASH_PARTITION_VERSION =  0x4
sys_parti_header_format = '<LLII'

mibib_crc_data_format = '<IIII'
FLASH_MIBIB_CRC_MAGIC1 = 0x9D41BEA1
FLASH_MIBIB_CRC_MAGIC2 = 0xF1DED2EA
FLASH_MIBIB_CRC_VERSION = 0x1

FLASH_PART_NAME_LENGTH = 16
sys_parti_entry_format = '<%dsIIBBBB' % FLASH_PART_NAME_LENGTH
user_parti_entry_format= '<%dsII' % FLASH_PART_NAME_LENGTH

user_partition_entries = []
token_stack = []
ignore_tags = ['comment', 'which_flash', 'img_name']
xml_comment_tag = '!--'
partitions_parent_tag_name = "partition"
full_file_length = 464
header = []
metadata = []
blockdata = []
footer = []
all_data = []



crc32_table = [  
  0x00000000,  0x04c11db7,  0x09823b6e,  0x0d4326d9,
  0x130476dc,  0x17c56b6b,  0x1a864db2,  0x1e475005,
  0x2608edb8,  0x22c9f00f,  0x2f8ad6d6,  0x2b4bcb61,
  0x350c9b64,  0x31cd86d3,  0x3c8ea00a,  0x384fbdbd,
  0x4c11db70,  0x48d0c6c7,  0x4593e01e,  0x4152fda9,
  0x5f15adac,  0x5bd4b01b,  0x569796c2,  0x52568b75,
  0x6a1936c8,  0x6ed82b7f,  0x639b0da6,  0x675a1011,
  0x791d4014,  0x7ddc5da3,  0x709f7b7a,  0x745e66cd,
  0x9823b6e0,  0x9ce2ab57,  0x91a18d8e,  0x95609039,
  0x8b27c03c,  0x8fe6dd8b,  0x82a5fb52,  0x8664e6e5,
  0xbe2b5b58,  0xbaea46ef,  0xb7a96036,  0xb3687d81,
  0xad2f2d84,  0xa9ee3033,  0xa4ad16ea,  0xa06c0b5d,
  0xd4326d90,  0xd0f37027,  0xddb056fe,  0xd9714b49,
  0xc7361b4c,  0xc3f706fb,  0xceb42022,  0xca753d95,
  0xf23a8028,  0xf6fb9d9f,  0xfbb8bb46,  0xff79a6f1,
  0xe13ef6f4,  0xe5ffeb43,  0xe8bccd9a,  0xec7dd02d,
  0x34867077,  0x30476dc0,  0x3d044b19,  0x39c556ae,
  0x278206ab,  0x23431b1c,  0x2e003dc5,  0x2ac12072,
  0x128e9dcf,  0x164f8078,  0x1b0ca6a1,  0x1fcdbb16,
  0x018aeb13,  0x054bf6a4,  0x0808d07d,  0x0cc9cdca,
  0x7897ab07,  0x7c56b6b0,  0x71159069,  0x75d48dde,
  0x6b93dddb,  0x6f52c06c,  0x6211e6b5,  0x66d0fb02,
  0x5e9f46bf,  0x5a5e5b08,  0x571d7dd1,  0x53dc6066,
  0x4d9b3063,  0x495a2dd4,  0x44190b0d,  0x40d816ba,
  0xaca5c697,  0xa864db20,  0xa527fdf9,  0xa1e6e04e,
  0xbfa1b04b,  0xbb60adfc,  0xb6238b25,  0xb2e29692,
  0x8aad2b2f,  0x8e6c3698,  0x832f1041,  0x87ee0df6,
  0x99a95df3,  0x9d684044,  0x902b669d,  0x94ea7b2a,
  0xe0b41de7,  0xe4750050,  0xe9362689,  0xedf73b3e,
  0xf3b06b3b,  0xf771768c,  0xfa325055,  0xfef34de2,
  0xc6bcf05f,  0xc27dede8,  0xcf3ecb31,  0xcbffd686,
  0xd5b88683,  0xd1799b34,  0xdc3abded,  0xd8fba05a,
  0x690ce0ee,  0x6dcdfd59,  0x608edb80,  0x644fc637,
  0x7a089632,  0x7ec98b85,  0x738aad5c,  0x774bb0eb,
  0x4f040d56,  0x4bc510e1,  0x46863638,  0x42472b8f,
  0x5c007b8a,  0x58c1663d,  0x558240e4,  0x51435d53,
  0x251d3b9e,  0x21dc2629,  0x2c9f00f0,  0x285e1d47,
  0x36194d42,  0x32d850f5,  0x3f9b762c,  0x3b5a6b9b,
  0x0315d626,  0x07d4cb91,  0x0a97ed48,  0x0e56f0ff,
  0x1011a0fa,  0x14d0bd4d,  0x19939b94,  0x1d528623,
  0xf12f560e,  0xf5ee4bb9,  0xf8ad6d60,  0xfc6c70d7,
  0xe22b20d2,  0xe6ea3d65,  0xeba91bbc,  0xef68060b,
  0xd727bbb6,  0xd3e6a601,  0xdea580d8,  0xda649d6f,
  0xc423cd6a,  0xc0e2d0dd,  0xcda1f604,  0xc960ebb3,
  0xbd3e8d7e,  0xb9ff90c9,  0xb4bcb610,  0xb07daba7,
  0xae3afba2,  0xaafbe615,  0xa7b8c0cc,  0xa379dd7b,
  0x9b3660c6,  0x9ff77d71,  0x92b45ba8,  0x9675461f,
  0x8832161a,  0x8cf30bad,  0x81b02d74,  0x857130c3,
  0x5d8a9099,  0x594b8d2e,  0x5408abf7,  0x50c9b640,
  0x4e8ee645,  0x4a4ffbf2,  0x470cdd2b,  0x43cdc09c,
  0x7b827d21,  0x7f436096,  0x7200464f,  0x76c15bf8,
  0x68860bfd,  0x6c47164a,  0x61043093,  0x65c52d24,
  0x119b4be9,  0x155a565e,  0x18197087,  0x1cd86d30,
  0x029f3d35,  0x065e2082,  0x0b1d065b,  0x0fdc1bec,
  0x3793a651,  0x3352bbe6,  0x3e119d3f,  0x3ad08088,
  0x2497d08d,  0x2056cd3a,  0x2d15ebe3,  0x29d4f654,
  0xc5a92679,  0xc1683bce,  0xcc2b1d17,  0xc8ea00a0,
  0xd6ad50a5,  0xd26c4d12,  0xdf2f6bcb,  0xdbee767c,
  0xe3a1cbc1,  0xe760d676,  0xea23f0af,  0xeee2ed18,
  0xf0a5bd1d,  0xf464a0aa,  0xf9278673,  0xfde69bc4,
  0x89b8fd09,  0x8d79e0be,  0x803ac667,  0x84fbdbd0,
  0x9abc8bd5,  0x9e7d9662,  0x933eb0bb,  0x97ffad0c,
  0xafb010b1,  0xab710d06,  0xa6322bdf,  0xa2f33668,
  0xbcb4666d,  0xb8757bda,  0xb5365d03,  0xb1f740b4 ]

def crc_32_calc(buf_ptr):
    seed = 0
    buf_len = len(buf_ptr) * 8
    data = None
    crc = None

    # Generate the CRC by looking up the transformation in a table and
    # XOR-ing it into the CRC, one byte at a time.
    i = 0
    crc = seed
    while (buf_len >= 8):
        crc = crc32_table[ ( (( crc >> 24 ) & 0xffffffff) ^ ( ord(buf_ptr[i]) & 0xff ) ) ] ^ (( crc << 8 ) & 0xffffffff)
        buf_len -= 8
        i += 1

    # Finish calculating the CRC over the trailing data bits. This is done by
    # aligning the remaining data bits with the CRC MSB, and then computing the
    # CRC one bit at a time.
    if ( buf_len != 0 ):
        print "we don't support this right now!"
        sys.exit(1)

    return (crc & 0xFFFFFFFF)


def prettify(elem):
    """Return a pretty-printed XML string for the Element.
    """
    rough_string = ET.tostring(elem, 'utf-8')
    reparsed = minidom.parseString(rough_string)
    return reparsed.toprettyxml(indent="  ")
    
def InitializePatchXMLFileVars():
    global PatchesXML
    PatchesXML = Element('patches')
    PatchesXML.append(Comment('NOTE: This is an ** Autogenerated file **'))
    PatchesXML.append(Comment('NOTE: Patching is in little endian format, i.e. 0xAABBCCDD will look like DD CC BB AA in the file or on disk'))

def UpdatePatch(StartSector,page_size_in_kb,ByteOffset,PHYPartition,size_in_bytes,szvalue,szfilename,szwhat):
    global PatchesXML

    SubElement(PatchesXML, 'patch', {'start_sector':StartSector, 'byte_offset':ByteOffset,
                                     'physical_partition_number':str(PHYPartition), 'size_in_bytes':str(size_in_bytes),
                                     'value':szvalue, 'filename':szfilename, 'SECTOR_SIZE_IN_BYTES':str(page_size_in_kb * 1024), 'what':szwhat   })

def ParseToCreateOutputFiles(filename, block_size_in_kb, page_size_in_kb, total_page_count):
    global NumSectorsUsed,user_partition_entries,usr_part_magic1,usr_part_magic2,partition_version

    try:
        root = ET.parse( filename ) ## root = ET.parse( "temp_program.xml" )
    except Exception, x:
        common.port_logger.error("XML PARSE failure for '%s', Reason: '%s'" % (filename,x))
        sys.exit(1)

    RootTag = "data"
    iter = root.getiterator()
    elem = Element("program")

        
    for element in iter:
        
        if element.tag=="usr_part_magic1":
            usr_part_magic1 = int(element.text,16)
        elif element.tag=="usr_part_magic2":
            usr_part_magic2 = int(element.text,16)
        elif element.tag=="partition_version":
            partition_version = int(element.text,16)

        ##print element.tag   ## 		<partition>
                        ##          <name length="16" type="string">0:SDI</name>
        if element.tag=="partition":
            ## Found a <partition> element  THIS MUST BE FIRST, OR ELSE NOT FORMED CORRECTLY
            user_partition_entries.append({"label":"","size_in_kb":0,"padding_in_kb":0,"filename":"","attr":0,"which_flash":0,"num_partition_sectors":0,"start_sector":0,"last_sector":0})           ## make room for it
            NumPartitions = len(user_partition_entries) ## 
        elif element.tag=="name":
            user_partition_entries[NumPartitions - 1]["label"]      = element.text
        elif element.tag=="img_name":
            user_partition_entries[NumPartitions - 1]["filename"]   = element.text
        elif element.tag=="size_blks":
            user_partition_entries[NumPartitions - 1]["size_in_kb"]     += (int(element.text,16)*(block_size_in_kb))
        elif element.tag=="pad_blks":
            user_partition_entries[NumPartitions - 1]["padding_in_kb"]  += (int(element.text,16)*(block_size_in_kb))
        elif element.tag=="size_kb":    ## KILOBYTES
            if element.text == '0xFFFFFFFF':
                # this conversion is added to handle size_kb=0xFFFFFFFF
                element.text = 0
            user_partition_entries[NumPartitions - 1]["size_in_kb"]     += int(element.text)
        elif element.tag=="pad_kb":
            if element.text == '0xFFFF':
                # this conversion is added to handle pad_kb=0xFFFF
                element.text = 0
            user_partition_entries[NumPartitions - 1]["padding_in_kb"]  += int(element.text)
        elif element.tag=="attr":
            user_partition_entries[NumPartitions - 1]["attr"]           = user_partition_entries[NumPartitions - 1]["attr"]<<8 | int(element.text,16)
        elif element.tag=="attr":
            user_partition_entries[NumPartitions - 1]["which_flash"]    = int(element.text)


        #if element.keys():
        #    for name, value in element.items():
        #        print "\tKEY '%s' = '%s'" % (name,value)
        #if element.text:
        #    print element.text

    ## Create RAWPROGRAM

    NewXML = Element(RootTag)    ## elem = tree.getroot(), typically it's '<data>' and type(root.getroot().tag) is <type 'str'>
    NewXML.append(Comment('NOTE: This is an ** Autogenerated file **'))
    NewXML.append(Comment('BlockSize = %d KB' % block_size_in_kb))
    NewXML.append(Comment('PageSize  = %d KB' % page_size_in_kb))
    NewXML.append(Comment('NUM_PARTITION_SECTORS  = %d' % total_page_count))
    
    # elem = Element("erase")
    # elem.attrib["num_partition_sectors"]    =  str(4294967295)
    # elem.attrib["start_sector"]             =  str(0)
    # NewXML.append(elem)

    NumPartitions = len(user_partition_entries) ## 
    NumSectorsUsed= 0

    sys_parti_buffer = struct.pack(sys_parti_header_format, FLASH_PART_MAGIC1, FLASH_PART_MAGIC2, FLASH_PARTITION_VERSION, NumPartitions)
    user_parti_buffer= struct.pack(sys_parti_header_format, usr_part_magic1, usr_part_magic2, partition_version, NumPartitions)

    #initialize for patch.xml
    InitializePatchXMLFileVars()
    Patch_StartSector = 1
    Patch_CRCStartSector = 3
    CRCByteOffset = 12
    
    for i in range(NumPartitions):

        size_in_kb      = user_partition_entries[i]["size_in_kb"]
        padding_in_kb   = user_partition_entries[i]["padding_in_kb"]

        ## Round to nearest page size
        num_blocks_needed = size_in_kb/block_size_in_kb
        if size_in_kb%block_size_in_kb!=0:
            num_blocks_needed+=1
        if i==NumPartitions-1 and size_in_kb==0:
            total_page_count = total_page_count - NumSectorsUsed
            num_blocks_needed = (total_page_count*page_size_in_kb/block_size_in_kb)

        num_padding_blocks_needed = padding_in_kb / block_size_in_kb

        if padding_in_kb>0 and padding_in_kb % block_size_in_kb:
            num_padding_blocks_needed += 1

        num_blocks_needed += num_padding_blocks_needed

        elem = Element("program")
        elem.attrib["start_sector"]             = "%d" % NumSectorsUsed
        elem.attrib["SECTOR_SIZE_IN_BYTES"]     = "%d" % (page_size_in_kb*1024)
        elem.attrib["PAGES_PER_BLOCK"]          = "%d" % (block_size_in_kb/page_size_in_kb)
        elem.attrib["physical_partition_number"]= "0"
        elem.attrib["num_partition_sectors"]    = "%d" % (num_blocks_needed*block_size_in_kb/page_size_in_kb)
        elem.attrib["last_sector"]    = "%d" % (NumSectorsUsed + num_blocks_needed*block_size_in_kb/page_size_in_kb - 1)
        # pdb.set_trace()
        temp_label = user_partition_entries[i]["label"].split(":")
        elem.attrib["label"] = temp_label[len(temp_label)-1]

        #if i == NumPartitions-1:
        #    elem.attrib["num_partition_sectors"] = "%d" % 0xFFFFFFFF   ## don't do this since need to send that many sectors via USB ??
      
        partition_filename = "partition_complete_p" + str(page_size_in_kb) + "K_b" + str (block_size_in_kb) + "K.mbn"
        if user_partition_entries[i]["filename"] == "partition.mbn":
            elem.attrib["filename"]             = partition_filename
        else:
            elem.attrib["filename"]             = user_partition_entries[i]["filename"]

        NewXML.append(elem)

        sys_parti_buffer  += struct.pack(sys_parti_entry_format, user_partition_entries[i]["label"], NumSectorsUsed/(block_size_in_kb/page_size_in_kb), num_blocks_needed, (user_partition_entries[i]["attr"]>>24)&0xFF, (user_partition_entries[i]["attr"]>>16)&0xFF, (user_partition_entries[i]["attr"]>>8)&0xFF, user_partition_entries[i]["which_flash"])
        user_parti_buffer += struct.pack(user_parti_entry_format, user_partition_entries[i]["label"], num_blocks_needed-num_padding_blocks_needed, num_padding_blocks_needed)
        user_parti_buffer += struct.pack('>I', (user_partition_entries[i]["attr"] & 0xFFFFFFFE))    ## force FE to mean in blocks

        #create entries in patch.xml for mibib partition
        if i==NumPartitions-1 and size_in_kb==0:
            UpdatePatch(str(Patch_StartSector),page_size_in_kb,str(len(sys_parti_buffer) - 8),0,4,"NUM_DISK_BLOCKS-%d." % (NumSectorsUsed*page_size_in_kb/block_size_in_kb),partition_filename,"Update MIBIB partition last sector size limit")
            UpdatePatch(str(Patch_CRCStartSector),page_size_in_kb,str(CRCByteOffset),0,4,"CRC32(0,%d)" % (page_size_in_kb*2*1024),partition_filename, "Update MIBIB partition with CRC of MIBIB header sector and Syster partition sector.") # CRC32(start_sector:num_bytes)
            
        NumSectorsUsed += (num_blocks_needed*block_size_in_kb/page_size_in_kb)

    if (len(sys_parti_buffer)) > (page_size_in_kb*1024):
        print "sys_parti_buffer exceeded page length!"
        sys.exit(1)
    else:
        sys_parti_buffer = sys_parti_buffer + '\xff' * ((page_size_in_kb*1024) - len(sys_parti_buffer))


    TrimmedXMLfileStr = prettify(NewXML)

    try:
        rawprogram_filename = "rawprogram_nand_p" + str(page_size_in_kb) + "K_b" + str (block_size_in_kb) + "K.xml"
        out_fp = open(rawprogram_filename, 'w')
    except Exception, x:
        print "ERROR: could not open 'rawprogram_nand.xml' for writing, Reason: '%s'" % x
        return

    out_fp.write(TrimmedXMLfileStr)

    out_fp.close()
    print "Created '%s'" % rawprogram_filename

    mibib_magic_buffer = struct.pack(mi_boot_info_block_format, MIBIB_MAGIC1, MIBIB_MAGIC2, MIBIB_VERSION, 1)
    if (len(mibib_magic_buffer)) > (page_size_in_kb*1024):
        print "len(mibib_magic_buffer) exceeded page length!"
        sys.exit(1)
    else:
        mibib_magic_buffer = mibib_magic_buffer + '\xff' * ((page_size_in_kb*1024) - len(mibib_magic_buffer))


    if (len(user_parti_buffer)) > (page_size_in_kb*1024):
        print "user_parti_buffer exceeded page length!"
        sys.exit(1)
    else:
        user_parti_buffer = user_parti_buffer + '\xff' * ((page_size_in_kb*1024) - len(user_parti_buffer))

    user_parti_buffer = '\xff' * (page_size_in_kb*1024)

    crc_value = crc_32_calc(mibib_magic_buffer + sys_parti_buffer)
    crc_value = crc_value & 0xffffffff
    crc_buffer = struct.pack(mibib_crc_data_format, FLASH_MIBIB_CRC_MAGIC1, FLASH_MIBIB_CRC_MAGIC2, FLASH_MIBIB_CRC_VERSION, crc_value)

    if (len(crc_buffer)) > (page_size_in_kb*1024):
        print "crc_buffer exceeded page length!"
        sys.exit(1)
    else:
        crc_buffer = crc_buffer + '\xff' * ((page_size_in_kb*1024) - len(crc_buffer))

    try:
        fp = open(partition_filename, 'wb')
    except Exception, x:
        print "ERROR: could not open '%s' for writing, Reason: '%s'" % (partition_filename, x)
        return
    fp.write(mibib_magic_buffer + sys_parti_buffer + user_parti_buffer + crc_buffer)
    fp.close()
    print "Created '%s'" % partition_filename
    
    try:
        patch_filename = "patch_p" + str(page_size_in_kb) + "K_b" + str (block_size_in_kb) + "K.xml"
        fp = open(patch_filename, 'w')
    except Exception, x:
        print "ERROR: could not open 'patch.xml' for writing, Reason: '%s'" % x
        return
    fp.write(prettify(PatchesXML))
    fp.close()
    print "Created '%s'" % patch_filename
    


def clean_string(some_string):
    from string import printable
    some_string = "".join([ ch for ch in some_string if ch in printable ])
    some_string = some_string.replace("\n", "")
    some_string = some_string.replace("\r", "")
    return some_string

def strip_hex(hex_string):
    if len(hex_string) < 3 or not (hex_string[:2] == "0x" or hex_string[:2] == "0X"):
        return hex_string	
    return hex_string[2:]

def little_endian(hex_string):
    if len(hex_string) % 2 != 0:
        print "%s is an invalid value to convert to little endian!" % (hex_string)
        return
    swap_list = []
    for i in range(0, len(hex_string), 2):
        swap_list.insert(0, hex_string[i:i+2])
    return ''.join(swap_list)

def get_attribute(token, att_name):
    tag = get_tag_name(token)
    tag_att_list = re.split('\s+', strip_tags(token))
    tag_att_list.remove(tag)
    for att in tag_att_list:
        att_val = att.split("=")
        if len(att_val) == 2 and att_val[0] == att_name:
            return att_val[1].strip('"').strip("'")
    return None

def pad_string(token, length, rightjust=True):
    if len(token) >= length:
        return token
    else:
        return (rightjust and token.rjust(length, '0') or token.ljust(length, '0'))

def is_tag(token):
    return (len(token) > 2 and token[0] == '<' and token[-1] == '>')

def strip_tags(token):
    if is_tag(token):
        return token[1:-1]

def is_closing_tag(token):
    return (is_tag(token) and token[1] == '/')

def get_tag_name(tag):
    if is_closing_tag(tag):
        return tag[2:-1]
    else:
        tag_split = re.split('\s+', tag[1:-1])
        return tag_split[0]

def process_tag(tag):
    tag_name = get_tag_name(tag)	
    parent_tag_name = len(token_stack) >= 2 and get_tag_name(token_stack[-2]) or ""
        
    if is_closing_tag(tag):
        if tag_name == partitions_parent_tag_name:
            if len(metadata) == 0:
                metadata.append(1)
            else:
                metadata[0] = metadata[0] + 1

def process_data(data, token_stack):
    tag = token_stack[-1]
    tag_name = get_tag_name(tag)
    parent_tag_name = len(token_stack) >= 2 and get_tag_name(token_stack[-2]) or ""
    
    if tag_name in ignore_tags:
        return

    clean_data = data
    data_length = get_attribute(tag, "length")
    data_type = get_attribute(tag, "type")
    if not data_type == "string":
        clean_data = "%02X" % eval(clean_data)
    else:
        clean_data = "".join(["%02x" % ord(x) for x in clean_data])

    if not data_length is None:
        data_length = int(data_length)
        clean_data = pad_string(clean_data, data_length * 2, data_type != "string")
    
    data_endianness = get_attribute(tag, "endian")
    # assuming big-endian by default
    # assuming 'string's need to be converted to little endian
    if not data_type == "string" or ((not data_endianness is None) and data_endianness.lower() == "little"):
        clean_data = little_endian(clean_data)
    
    if not parent_tag_name == partitions_parent_tag_name:
        header.append(clean_data)
    else:
        blockdata.append(clean_data)

def generate_metadata():
    if len(metadata) > 0:
        metadata[0] = little_endian(pad_string("%02X" % metadata[0], 4*2))

def generate_footer():
    current_file_len = 0
    for x in all_data:
        current_file_len  = current_file_len + len(x)/2
    footer.append("0" * 2 * (full_file_length - current_file_len))

def write_bin_file(filename, data):
    fp = open(filename, 'wb')
    for i in range(0, len(data), 2):
        data_item = data[i:i+2]
        fp.write(chr(int(data_item, 16)))
    fp.close()
    print "\nCreated '%s'" % filename

#main

if len(sys.argv) < 3:
    print "Usage: python nand_mbn_generator.py <partitionFileName> <block size in KB> <page size in KB> <NUM_PARTITION_SECTORS>\n"
    print "Ex: python nand_mbn_generator.py partition_nand.xml partition.mbn 256 4 131072"
    print "Ex: python nand_mbn_generator.py partition_nand.xml partition.mbn 128 2 131072"
    sys.exit(1);

filename = sys.argv[1]
bin_file_name = sys.argv[2]

#input for block_size_in_kb
if len(sys.argv) >= 4:
    block_size_in_kb=int(sys.argv[3])
else:
    block_size_in_kb=256

#input for page_size_in_kb
if len(sys.argv) >= 5:
    page_size_in_kb=int(sys.argv[4])
else:
    page_size_in_kb=4

#input for total_page_count    
if len(sys.argv) >= 6:
    total_page_count=int(sys.argv[5])
else:
    total_page_count=131072

file_string = open(filename, 'r').read()
file_string = re.sub(r'>\s+', ">", file_string)
file_string = re.sub(r'\s+<', "<", file_string)
file_string = clean_string(file_string)

# A token is either a tag i.e. <[^>\s]*> or it is a (non-tag) value i.e. [^<>]+
#splitter = re.compile('(<\/?\S+[^>]*>|[^<>]+)')
splitter = re.compile('(<[^>]*>|[^<>]+)')
token_list = splitter.findall(file_string)

for token in token_list:
    #print "Processing", token
    if is_tag(token):
        if get_tag_name(token) == xml_comment_tag or get_tag_name(token) == "?xml":
            continue
        if is_closing_tag(token):
            token_stack.pop()
        else:
            token_stack.append(token)
        process_tag(token)
    else:
        process_data(token, token_stack)

generate_metadata()

all_data.extend(header)
all_data.extend(metadata)
all_data.extend(blockdata)

generate_footer()

all_data.extend(footer)
all_data = "".join(all_data)
write_bin_file(bin_file_name, all_data)

ParseToCreateOutputFiles(filename, block_size_in_kb, page_size_in_kb, total_page_count)
