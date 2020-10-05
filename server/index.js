const express = require('express');
const cors = require('cors');
const AzureProxy = require('./AzureProxy');

// Create a config.js file with the configured pipepline
const config = require('./config');

const port = 3003;
const app = express();

app.use(cors());
app.get('/', (req, res) => {
  Promise.all(
    config.map((c) => new AzureProxy(c).getLatestStates(1)),
  )
    .then((data) => {
      const response = [];
      for (const definition of data) {
        for (const state of definition.states) {
          response.push({ d: definition.pipeline, s: state });
        }
      }
      res.json(response);
    });
});

app.listen(port, () => {
  console.log(`server is listening on http://localhost:${port}`);
});
