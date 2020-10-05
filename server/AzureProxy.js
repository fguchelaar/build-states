const fetch = require('node-fetch');
const urljoin = require('url-join');

function AzureProxy(options) {
  this.baseUrl = options.baseURL;
  this.project = options.project;
  this.definitionId = options.buildDefinition;
  this.token = options.token;
  this.mode = options.mode;
}

AzureProxy.prototype.getLatestStates = function getLatestStates(numberOfBuilds) {
  const api = this.mode === 'build' ? '/_apis/build/builds' : '/_apis/release/deployments';
  const queryString = this.mode === 'build' ? '?definitions=' : '?definitionId=';

  const requestUrl = urljoin(
    this.baseUrl,
    this.project,
    api,
    queryString + this.definitionId,
    `&$top=${numberOfBuilds}`,
    `&maxBuildsPerDefinition=${numberOfBuilds}`,
  );
  const auth = `Basic ${Buffer.from(`${this.token}:`).toString('base64')}`;
  console.log(requestUrl);
  return new Promise((resolve, reject) => {
    fetch(requestUrl, {
      headers: {
        Authorization: auth,
      },
    })
      .then((res) => res.json())
      .then((json) => this.parseBuildResults(json))
      .then((states) => resolve(states))
      .catch((err) => {
        reject(err);
      });
  });
};

AzureProxy.prototype.parseBuildResults = function parseBuildResults(json) {
  const values = json.value.sort((a, b) => a.id - b.id).reverse();

  // I'm gonna assume that every 'value' has the same definition, so we'll just
  // get the details of the first one.
  const pipeline = values.length === 0
    ? `No recent builds for ${this.project}.${this.definitionId}`
    : values[0].definition.name;

  if (this.mode === 'release') {
    const states = values.map((val) => {
      if (val.deploymentStatus === 'failed') {
        return 'FAILURE';
      } if (val.deploymentStatus === 'succeeded') {
        return 'SUCCESS';
      } if (val.deploymentStatus === 'canceled') {
        return 'CANCELED';
      } if (val.deploymentStatus === 'inProgress') {
        return 'BUILDING';
      }
      return 'ERROR';
    });

    return {
      pipeline,
      states,
    };
  }

  const states = values.map((val) => {
    if (val.status === 'completed') {
      if (val.result === 'failed') {
        return 'FAILURE';
      } if (val.result === 'succeeded') {
        return 'SUCCESS';
      } if (val.result === 'partiallySucceeded') {
        return 'PARTIAL';
      } if (val.result === 'canceled') {
        return 'CANCELED';
      }
      return 'ERROR';
    } if (val.status === 'inProgress') {
      return 'BUILDING';
    } if (val.status === 'notStarted') {
      return 'NOTSTARTED';
    }
    return 'ERROR';
  });

  return {
    pipeline,
    states,
  };
};

module.exports = AzureProxy;
