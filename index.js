const express = require('express');
const bodyParser = require('body-parser');
const cors = require('cors');

const app = express();

// Middleware setup
app.use(cors());
// Increase payload limit and add proper error handling for JSON parsing
app.use(bodyParser.json({ limit: '10mb' }));
app.use(bodyParser.urlencoded({ extended: true }));

let latestSensorData = null;

// Debug middleware to log all requests
app.use((req, res, next) => {
    console.log(`${req.method} ${req.path}`);
    console.log('Headers:', req.headers);
    console.log('Body:', req.body);
    next();
});

// POST endpoint for soil data
app.post('/soil', (req, res) => {
    try {
        console.log('Received POST request to /soil');
        console.log('Request body:', req.body);

        // Check if we received the expected data
        if (!req.body || (!req.body.moisture && !req.body.rawValue)) {
            console.log('Invalid data received');
            return res.status(400).json({ 
                error: 'Invalid data format',
                expected: { moisture: 'number', rawValue: 'number' },
                received: req.body 
            });
        }

        // Store the sensor data
        latestSensorData = {
            moisture: req.body.moisture,
            rawValue: req.body.rawValue,
            timestamp: new Date().toISOString()
        };
        
        console.log('Stored sensor data:', latestSensorData);
        res.status(200).json({
            message: "Data received successfully",
            data: latestSensorData
        });
    } catch (error) {
        console.error('Error processing request:', error);
        res.status(500).json({ error: error.message });
    }
});

// GET endpoint for soil data
app.get('/soil', (req, res) => {
    try {
        console.log('Received GET request to /soil');
        const value = req.query.value;
        
        if (value) {
            latestSensorData = {
                value: value,
                timestamp: new Date().toISOString()
            };
        }
        
        res.status(200).json(latestSensorData || { message: "No data available" });
    } catch (error) {
        console.error('Error processing request:', error);
        res.status(500).json({ error: error.message });
    }
});

// Status endpoint
app.get('/status', (req, res) => {
    res.status(200).json({
        serverTime: new Date().toISOString(),
        lastReading: latestSensorData,
        status: 'running'
    });
});

// Root endpoint for health check
app.get('/', (req, res) => {
    res.status(200).json({
        status: 200,
        message: 'Server is running',
        endpoints: {
            post_soil: '/soil - POST with JSON body {moisture: number, rawValue: number}',
            get_soil: '/soil - GET with optional query parameter "value"',
            status: '/status - GET server status'
        }
    });
});

// Error handling for malformed JSON
app.use((err, req, res, next) => {
    console.error('Error:', err);
    if (err instanceof SyntaxError && err.status === 400 && 'body' in err) {
        return res.status(400).json({ 
            error: 'Invalid JSON format',
            message: err.message 
        });
    }
    next(err);
});

// 404 handler
app.use((req, res) => {
    res.status(404).json({ 
        error: 'Not Found',
        message: `Cannot ${req.method} ${req.path}`
    });
});

// Start server
const PORT = 3000;
app.listen(PORT, () => {
    console.log(`Server is running on port ${PORT}...`);
    console.log(`Test the server with:`);
    console.log(`curl -X POST -H "Content-Type: application/json" -d '{"moisture":75,"rawValue":1024}' http://localhost:3000/soil`);
});