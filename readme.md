
# LabStor

A high-performance, lightweight, hierarchical, modular storage stack for Linux-based systems.

## 1. Dependencies

Linux 5.4.0-77-generic  
linux-headers-5.4+  
cmake 3.10 or higher  
C++17 compiler  
C11 compiler  
yaml-cpp

### Linux Headers
```
#Ubuntu/Debian
sudo apt install linux-headers-`uname -r`
#Red Hat
sudo yum install linux-headers-`uname -r`
```

### SCSPKG
```
git clone https://github.com/lukemartinlogan/scspkg.git
bash install.sh
source ~/.bashrc
```

### TCL

```
#Ubuntu/Debian
sudo apt install tcl-dev
#Red Hat
sudo yum install tcl-devel
```

### Modulefiles
```
scspkg create modules
cd `scspkg pkg-src modules`
curl -LJO https://github.com/cea-hpc/modules/releases/download/v4.7.1/modules-4.7.1.tar.gz
tar xfz modules-4.7.1.tar.gz
cd modules-4.7.1
./configure --prefix=`scspkg pkg-root modules`
make
make install
echo "source \`scspkg pkg-root modules\`/init/bash" >> ~/.bashrc
echo "module use \`scspkg modules-path\`" >> ~/.bashrc
source ~/.bashrc
```

### MPICH
```
scspkg create mpich
cd `scspkg pkg-src mpich`
wget http://www.mpich.org/static/downloads/3.2/mpich-3.2.tar.gz --no-check-certificate
tar -xzf mpich-3.2.tar.gz
cd mpich-3.2
./configure --prefix=`scspkg pkg-root mpich` --enable-fast=O3 --enable-romio --enable-shared --with-pvfs2=`scspkg pkg-root orangefs` --with-file-system=pvfs2
make -j8
make install
```

### SCSBENCH
```
git clone https://github.com/lukemartinlogan/scsbench.git
python3 -m pip install -r requirements.txt  
python3 setup.py develop --user
```

### YAML-CPP

```
scspkg create yaml-cpp
cd `scspkg pkg-src yaml-cpp` 
git clone https://github.com/jbeder/yaml-cpp.git
cd yaml-cpp
git checkout db6deedcd301754723065e0bbb1b75927c5b49c7
mkdir build
cd build
cmake ../  -DYAML_BUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=`scspkg pkg-root yaml-cpp`
make -j8
make install
```

### LIBURING

```
scspkg create liburing
cd `scspkg pkg-src liburing`
git clone https://github.com/axboe/liburing.git
cd liburing
./configure --prefix=`scspkg pkg-root liburing`
make -j8
make install
```

### LIBAIO

```
sudo apt install libaio-dev
```

### SPDK

```
scspkg create spdk
git clone https://github.com/spdk/spdk
cd spdk
git submodule update --init
sudo scripts/pkgdep.sh
#Allocate huge pages & unbind NVMes
sudo HUGEMEM=2048 scripts/setup.sh
#Rebind NVMes
sudo scripts/setup.sh reset
#
./configure --prefix=`scspkg pkg-src spdk`
make -j8
make install
```

## 2. Building

```
cd /path/to/labstor
mkdir build  
cd build
cmake ../
make -j4  
```

## 3. Running
```
module load mpi yaml-cpp
make start_kernel_server
make start_trusted_server
make stop_trusted_server
make stop_kernel_server
```

### Random Notes

```
IP=192.168.56.1
ssh llogan@$IP -p 4632
```

```
Increase debugging level:
echo "7" > /proc/sys/kernel/printk

Decrease debugging level:
echo "4" > /proc/sys/kernel/printk
```
