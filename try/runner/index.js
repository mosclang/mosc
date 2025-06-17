const express = require('express')
const app = express()
const port = 3004

app.use(express.static('public'))
app.use('/src/', express.static('../../src'))


app.listen(port, () => {
  console.log(`Runner listening on ${port}`)
})