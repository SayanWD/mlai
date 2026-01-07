# MLAI - Machine Learning from Scratch

A complete machine learning library built from scratch in C for handwritten digit recognition using the MNIST dataset.

## Features

- **Custom Matrix Operations**: Efficient row-major matrix implementation
- **Automatic Differentiation**: Computational graph with reverse-mode autodiff
- **Neural Network Training**: SGD optimizer with mini-batch support
- **Memory Safety**: Arena-based allocator with security hardening
- **Cross-Platform**: Windows and Linux support

## Quick Start

### Prerequisites

- GCC or Clang compiler
- Python 3.7+ with pip
- Make (optional but recommended)

### Installation

1. Clone this repository:
```bash
git clone <repository-url>
cd mlai
```

2. Install Python dependencies:
```bash
pip install -r requirements.txt
```

3. Prepare the MNIST dataset:
```bash
make data
```

### Building

**Debug build (recommended for development):**
```bash
make debug
```

**Release build (optimized):**
```bash
make release
```

**With sanitizers (for testing):**
```bash
make asan
```

### Running

```bash
./mlai
```

The program will:
1. Load MNIST training and test data
2. Display a sample digit
3. Train a neural network for 10 epochs
4. Show training progress and final accuracy

Expected accuracy: ~86-89% on test set

## Project Structure

```
mlai/
├── main.c          # Core implementation
├── arena.c         # Memory allocator
├── prng.c          # Random number generator
├── *.h             # Header files
├── mnist.py        # Data preparation script
├── Makefile        # Build system
└── README.md       # This file
```

## Model Architecture

- **Input Layer**: 784 neurons (28×28 pixel images)
- **Hidden Layer 1**: 16 neurons with ReLU activation
- **Hidden Layer 2**: 16 neurons with ReLU + residual connection
- **Output Layer**: 10 neurons with Softmax (digit classes 0-9)
- **Loss Function**: Cross-entropy
- **Optimizer**: Stochastic Gradient Descent (SGD)

## Security Features

This project includes security hardening:

- **Compiler Flags**: Stack protectors, FORTIFY_SOURCE, PIE
- **Input Validation**: Bounds checking on all file operations
- **Memory Safety**: NULL checks, overflow protection
- **Sanitizers**: AddressSanitizer and UndefinedBehaviorSanitizer support

See [SECURITY.md](SECURITY.md) for detailed security information.

## Development

### Build Targets

```bash
make              # Build with debug symbols
make release      # Optimized release build
make asan         # Build with AddressSanitizer
make test-asan    # Run with AddressSanitizer
make data         # Prepare MNIST dataset
make clean        # Remove build artifacts
make cleanall     # Remove build artifacts and data
make analyze      # Static analysis (requires cppcheck)
make security-check  # Run security checks
```

### Testing

Run with sanitizers during development:
```bash
make asan
./mlai
```

Memory leak detection:
```bash
valgrind --leak-check=full ./mlai
```

### Code Style

- C11 standard
- Functions: lowercase with underscores
- Types: lowercase with underscores
- Macros: UPPERCASE
- Use typedefs from base.h (u32, f32, etc.)

## Contributing

1. Follow the security guidelines in SECURITY.md
2. Test with sanitizers before submitting
3. Run static analysis: `make analyze`
4. Ensure all builds pass: `make clean && make release`

## Documentation

- [CLAUDE.md](CLAUDE.md) - Development guide for AI assistants
- [SECURITY.md](SECURITY.md) - Security policy and guidelines
- [MAIN_INFO.md](MAIN_INFO.md) - Original project documentation (Russian)

## Limitations

- Single-threaded execution
- No GPU acceleration
- Fixed architecture (not configurable at runtime)
- No model serialization
- Limited to MNIST dataset

## License

Based on https://github.com/Magicalbat/videos/tree/main/machine-learning

The PCG random number generator (prng.c/prng.h) is licensed under Apache License 2.0.

## References

- Original implementation: [Magicalbat/videos](https://github.com/Magicalbat/videos/tree/main/machine-learning)
- MNIST Dataset: [TensorFlow Datasets](https://www.tensorflow.org/datasets/catalog/mnist)
- PCG Random: [pcg-random.org](https://www.pcg-random.org)

## Performance

Training 10 epochs on MNIST (60,000 samples):
- CPU: ~1-2 minutes (depending on hardware)
- Memory: ~100 MB
- Accuracy: 86-89%

## Troubleshooting

**Error: Cannot open file 'train_images.mat'**
- Run `make data` to download and prepare the dataset

**Error: Matrix dimensions too large**
- Check available memory
- Reduce batch size in main.c

**Segmentation fault**
- Build with `make asan` to identify memory errors
- Check that data files are not corrupted

**Poor accuracy (<80%)**
- Ensure data preparation completed successfully
- Try different random seed by modifying prng initialization
- Adjust learning rate or number of epochs

## Support

For issues and questions:
- Check [CLAUDE.md](CLAUDE.md) for development guidance
- Review [SECURITY.md](SECURITY.md) for security-related issues
- Open an issue on the project repository
