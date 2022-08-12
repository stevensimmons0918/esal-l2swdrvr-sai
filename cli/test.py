import ctypes

print ("hello from python")
esai_vendor_api = ctypes.CDLL('libesal.so')
esai_vendor_api.VendorCreateVlan.restype = ctypes.c_int
esai_vendor_api.VendorCreateVlan.argtypes = [ctypes.c_uint16]
esai_vendor_api.VendorCreateVlan(101)
