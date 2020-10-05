const express = require('express');
const cors = require('cors');
const AzureProxy = require('./AzureProxy');

// Create a config.js file with the configured pipepline
const config = require('./config');

const port = 3003;
const app = express();

app.use(cors());
app.get('/', (req, res) => {
  const proxy = new AzureProxy(config);
  proxy.getLatestStates(20, (states) => {
    res.json({ offByOne: states });
  });
});

app.listen(port, () => {
  console.log(`server is listening on http://localhost:${port}`);
});
