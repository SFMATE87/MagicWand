import os
import numpy as np
import pandas as pd
import tensorflow as tf
from sklearn.model_selection import train_test_split

# 1. Configuration
DATASET_PATH = 'C:/Users/Asus/Documents/Projects/MagicWand_Project/dataset'
CLASSES = ['I', 'Z', 'O', 'Idle']
NUM_CLASSES = len(CLASSES)
TIME_STEPS = 100
FEATURES = 6 # ax, ay, az, gx, gy, gz

# 2. Data Loading Function
def load_data():
    X = [] # This will hold the 3D gesture data
    y = [] # This will hold the labels (0, 1, 2, 3)

    for label_idx, class_name in enumerate(CLASSES):
        class_dir = os.path.join(DATASET_PATH, class_name)
        
        for filename in os.listdir(class_dir):
            if filename.endswith('.csv'):
                filepath = os.path.join(class_dir, filename)
                
                try:
                    # 1. Read CSV
                    df = pd.read_csv(filepath)
                    
                    # 2. Force everything to numbers. If it sees 'clk_drv:0x00', it turns it into NaN (Not a Number)
                    df = df.apply(pd.to_numeric, errors='coerce')
                    
                    # 3. Drop any rows that contain a NaN value
                    df = df.dropna()
                    
                    # 4. Strict check: If dropping the bad row ruined the 100-step count, ignore the file entirely
                    if df.shape[0] == TIME_STEPS and df.shape[1] == FEATURES:
                        X.append(df.values)
                        y.append(label_idx)
                    else:
                        pass # Silently skip files that don't fit the 100x6 rule after cleaning
                        
                except Exception as e:
                    print(f"Skipping completely unreadable file {filename}")
    
    # Convert lists to Numpy arrays (Tensors)
    X = np.array(X, dtype=np.float32)
    y = np.array(y, dtype=np.float32)
    
    return X, y

print("Loading data...")
X, y = load_data()
X = X - np.mean(X, axis=1, keepdims=True) #for not looking the 1G
print(f"Input data (X) shape: {X.shape}")
print(f"Labels (y) shape: {y.shape}")

# 3. Building the Neural Network (Conv1D)
def build_model(input_shape, num_classes):
    model = tf.keras.Sequential([
        #Looking for small patterns
        tf.keras.layers.Conv1D(filters=32, kernel_size=3, activation='relu', input_shape=input_shape), 
        tf.keras.layers.MaxPooling1D(pool_size=2),

        #Bigger, complex patterns
        tf.keras.layers.Conv1D(filters=64, kernel_size=3, activation='relu'),
        tf.keras.layers.MaxPooling1D(pool_size=2),

        # 2D to 1D
        tf.keras.layers.Flatten(),

        #Dense layer
        tf.keras.layers.Dense(128, activation='relu'),
        tf.keras.layers.Dropout(0.5), #Prevent overfitting

        #Final
        tf.keras.layers.Dense(num_classes, activation='softmax')
    ])

    #learnong rules
    model.compile(optimizer='adam', loss='sparse_categorical_crossentropy', metrics=['accuracy'])

    return model

print("Building the model...")
single_sample_shape = (TIME_STEPS, FEATURES) # This is your (100, 6)
model = build_model(single_sample_shape, NUM_CLASSES)
model.summary()

#Splitting teh data
print("Splitting 80/20...")
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=30)

#Training loop
print("Training...")
history = model.fit(
    X_train, y_train,
    epochs=50,
    batch_size=32,
    validation_data=(X_test,y_test)
)

#Saving
model.save('magic_wand_model.h5')
print("\n Model saved as 'magic_wand_model.h5'")