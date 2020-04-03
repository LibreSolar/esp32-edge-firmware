<template>
  <v-container>
    <v-layout text-xs-center wrap>
      <v-flex xs12 sm6 offset-sm3>
        <v-card>
          <v-card-text>
            <v-container fluid grid-list-lg>
              <v-layout row wrap>
                <v-flex xs12
                  v-for="(value, name) in data_objects"
                  :key="name"
                >
                  <v-text-field
                    :label="name.split('_')[0]"
                    v-model="data_objects[name]"
                    :suffix="name.search('_') > 0 ? name.split('_')[1].replace('degC', '°C') : ''"
                    outlined
                  ></v-text-field>
                </v-flex>
              </v-layout>
            </v-container>
          </v-card-text>
          <v-btn color="primary" @click="set_color">
            <v-icon left>save_alt</v-icon> Apply
          </v-btn>
          <v-btn @click="set_color">
            <v-icon left>clear</v-icon> Cancel
          </v-btn>
        </v-card>
      </v-flex>
    </v-layout>
  </v-container>
</template>

<script>
export default {
  data() {
    return {
      data_objects: null
    }
  },
  mounted() {
    this.$ajax
      .get('/api/conf')
      .then(data => {
        this.data_objects = data.data
      })
      .catch(error => {
        console.log(error)
      })
  },
  methods: {
    set_color: function() {
      this.$ajax
        .post("/api/v1/light/brightness", {
          red: this.red,
          green: this.green,
          blue: this.blue
        })
        .then(data => {
          console.log(data);
        })
        .catch(error => {
          console.log(error);
        });
    }
  }
};
</script>
