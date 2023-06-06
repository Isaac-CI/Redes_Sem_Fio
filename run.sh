# First, you'll have to export NS3_DIR to the ns-3.38 directory in your machine
# run export NS3_DIR=<path-to-ns3>

# Then, give the script, permission to execute
# chmod +x run.sh

# 1. Copying the src folder to the NS3 directory
cp -r src/ ${NS3_DIR}/scratch

# 2. Entering the NS3 directory
cd ${NS3_DIR}

# 3. Running NS3
./ns3 run scratch/src/components/main.cc