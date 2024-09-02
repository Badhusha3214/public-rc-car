
const express = require('express');
const bodyParser = require('body-parser');

const app = express();
app.use(bodyParser.json());

let latestCommand = null;


app.post('/set-command', (req, res) => {
    const value = req.query.value;
    latestCommand = value;
    res.status(200).json({
        status: 200,
        message: `Command set to ${latestCommand}.`
    });
});


app.get('/status', (req, res) => {
    res.status(200).send(latestCommand);
});


app.get('/', (req, res) => {
    res.status(200).json({
        status: 200,
        message: `Server is running.`
    });
});


app.listen(3000, () => {
    console.log(`Server is running on port 3000...`);
});
