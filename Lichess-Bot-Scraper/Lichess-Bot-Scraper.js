/*
Copyright 2023 En-En-Code

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// Helper function parsing nd-json based off one by Thibault Duplessis @
// https://gist.github.com/ornicar/a097406810939cf7be1df8ea30e94f3e
// Also has ideas from MDN docs, like ReadableStreamDefaultReader sample &
// https://developer.mozilla.org/en-US/docs/Web/API/Fetch_API/Using_Fetch
const readStream = processLine => response => {
  const splitter = /\r?\n/;
  const decoder = new TextDecoder('utf-8');
  let buf = '';
  const reader = response.body.getReader();
  return new Promise((resolve, fail) => {
    reader.read()
      .then(function processChunk(res) {
        if (res.done) {
          if (buf.length > 0) processLine(JSON.parse(buf));
          return;
        }
        buf += decoder.decode(res.value, {stream: true});
        const parts = buf.split(splitter);
        buf = parts.pop();
        for (const i of parts.filter(p => p)) { processLine(JSON.parse(i)) };
        
        return reader.read().then(processChunk).catch(reason => {
          throw new Error(`Error occured: ${reason}`);
        });
    }).catch(reason => {
      throw new Error(`Error occured: ${reason}`);
    }).finally(() => {
      resolve(linkCollection);
    });
  });
};

/*
  GitHub username rules: https://www.npmjs.com/package/github-username-regex
  Not 100% true: https://github.com/shinnn/github-username-regex/issues/4
  
  GitHub repo names in my testing cannot be longer than 100 characters.
  Any characters not /[A-Za-z\d_.-]/ are converted to /-/.
*/
const githubRepoRegex =
  /(?:(?:gist.)?github.com\/[A-Za-z\d](?:[A-Za-z\d]|-(?!-)){0,38}\/[A-Za-z\d_.-]{1,100})/;

/*
  Though testing on https://gitlab.com/users/sign_up, I have determined:
    Usernames must be between 2 and 255 /[A-Za-z\d_-]/.
    Usernames cannot start with a hyphen.
  
  GitLab repo names I am taking a liberal guess for.
*/
const gitlabRepoRegex =
  /(?:gitlab.(?:com|io)\/[A-Za-z\d_][A-Za-z\d_-]{1,254}\/[A-Za-z\d_.-]+)/;

/*
  SourceForge has a somewhat different method of indicating a project,
  which I will generously assume has code inside of it.
  
  SourceForge website pages (the latter half of the regex), generally
  lead to a project's source, so it is included.
  
  Project name rules are liberal guesses.
*/
const sourceforgeRepoRegex =
  /(?:sourceforge.(?:net|io)\/projects\/[A-Za-z\d_.-]+)|(?:[A-Za-z\d_.-]+.sourceforge.(?:net|io))/;

/*
  Even more liberal guesses taken on these.
  
  I'm choosing a necessarily incomplete list of sites so that I still
  filter out many URIs which are unlikely to lead to code repositories.
*/
const otherGitRepoRegex =
  /(?:(?:bitbucket.(?:org|io)|codeberg.org)\/[A-Za-z\d_.-]+\/[A-Za-z\d_.-]+)/;

const allRepoRegex =
  new RegExp(githubRepoRegex.source + `|` + gitlabRepoRegex.source + `|` +
    sourceforgeRepoRegex.source + `|` + otherGitRepoRegex.source, 'gi');

let linkCollection = [];
const extractRepoLinks = account => {
  let repoList = [];
  let potentialRepoList = [];
  if (account != null && account.profile != null) {  
    // Find every match to the regex in the bio and links and append them
    if (account.profile.bio != null &&
      account.profile.bio.match(allRepoRegex) != null) {
    	potentialRepoList.push(...account.profile.bio.match(allRepoRegex));
    }
    if (account.profile.links != null &&
      account.profile.links.match(allRepoRegex) != null) {
    	potentialRepoList.push(...account.profile.links.match(allRepoRegex));
    }
    for (let potentialRepo of potentialRepoList) {
      // These repositories are not engines themselves, but API bridges
      // between the Lichess API and the UCI protocol, so ignore them.
      // Also include duplicate links within the same account.
      if (!potentialRepo.includes("github.com/ShailChoksi/lichess-bot") &&
        !potentialRepo.includes("github.com/Torom/BotLi") &&
        !repoList.includes(potentialRepo)) {
        repoList.push(potentialRepo);
      }
    }
  }
  if (repoList.length > 0) {
    // Append the Lichess profile prefix so the account becomes a link
    // Append the web protocol so most command line recognize them as links
    linkCollection.push(["https://lichess.org/@/".concat(account.id),
      repoList.map(x => "https://".concat(x))]);
  }
}

const scrapeRepoLinks = () => {
  return fetch(`https://lichess.org/api/bot/online`)
    .then(readStream(extractRepoLinks))
}

scrapeRepoLinks().then(out => console.log(out));
